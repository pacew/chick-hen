#include "chick-hen.h"

#define min(a,b) ((a)<(b)?(a):(b))

#define MAX_CHANS 10
#define RAW_CHANLIST_SIZE (((MAX_CHANS * PROTO_CHANLIST_NBITS) + 7) / 8)

int my_mac_hash;

int vflag;
int xflag;

#define DATA_INTERVAL_SECS 1
#define XMIT_INTERVAL_SECS 1
#define LISTEN_INTERVAL_SECS 0.5

double last_xmit_ts;
double listen_until_ts;
double next_data_ts;

struct nvram {
	uint32_t sig;
	unsigned char hen_key[HEN_KEY_LEN];
	uint16_t chanlist_used;
	unsigned char chanlist[RAW_CHANLIST_SIZE];
} __attribute__((packed));


struct nvram nvram, nvram_saved;

char nvram_filename[100];

int dpoint_bits;
int dpoint_bytes;

void
print_chanlist (void)
{
	struct proto_buf cpb;
	struct proto_chanlist chanlist;
	
	proto_decode_init (&cpb, nvram.chanlist, nvram.chanlist_used);
	while (cpb.used_bits + PROTO_CHANLIST_NBITS <= cpb.avail_bits) {
		proto_decode (&cpb, &proto_chanlist_desc, &chanlist);
		printf ("%3d %3d %3d %3d\n",
			chanlist.chan_type,
			chanlist.port,
			chanlist.bit_width,
			chanlist.bit_position);
	}
}

void
nvram_parse (void)
{
	struct proto_buf pb;
	struct proto_chanlist chanlist;

	printf ("nvram parse: chanlist\n");
	
	dpoint_bits = 0;
	proto_decode_init (&pb, nvram.chanlist, nvram.chanlist_used);
	while (pb.used_bits + PROTO_CHANLIST_NBITS <= pb.avail_bits) {
		proto_decode (&pb, &proto_chanlist_desc, &chanlist);
		dpoint_bits += chanlist.bit_width;
	}
	dpoint_bytes = (dpoint_bits + 7) / 8;

	print_chanlist();
}

void
nvram_startup (void)
{
	FILE *f;
	int success;
	uint32_t stored_sig;
	
	success = 0;
	
	sprintf (nvram_filename, "nvram%02x%02x%02x%02x%02x%02x",
		 my_mac_addr[0], my_mac_addr[1], my_mac_addr[2],
		 my_mac_addr[3], my_mac_addr[4], my_mac_addr[5]);

	if ((f = fopen (nvram_filename, "r")) == NULL)
		goto done;
	
	if (fread (&nvram, sizeof nvram, 1, f) != 1)
		goto done;
	nvram_saved = nvram;

	stored_sig = nvram.sig;
	nvram.sig = 0;

	if (simple_digest (&nvram, sizeof nvram) != stored_sig) {
		printf ("nvram cksum error\n");
		goto done;
	}

	nvram.sig = stored_sig;

	success = 1;

done:
	if (! success)
		memset (&nvram, 0, sizeof nvram);

	nvram_parse ();

	if (f)
		fclose (f);
}	

void
nvram_save (void)
{
	char name[1000];
	FILE *outf;
	
	if (memcmp (&nvram, &nvram_saved, sizeof nvram) == 0)
		return;
	
	printf ("saving updated nvram\n");
	nvram.sig = 0;
	nvram.sig = simple_digest (&nvram, sizeof nvram);

	sprintf (name, "%s.new", nvram_filename);
	remove (name);
	if ((outf = fopen (name, "w")) == NULL) {
		fprintf (stderr, "can't create %s\n", name);
	} else {
		fwrite (&nvram, sizeof nvram, 1, outf);
		fclose (outf);
		rename (name, nvram_filename);
		nvram_saved = nvram;
	}
}
		 
void
collect_dpoint (void)
{
	struct proto_buf dpb, cpb;
	struct proto_chanlist chanlist;
	int width, mask;
	int val;
	
//	proto_encode_init (&dpb, dbuf, N);

	proto_decode_init (&cpb, nvram.chanlist, nvram.chanlist_used);
	while (cpb.used_bits + PROTO_CHANLIST_NBITS <= cpb.avail_bits) {
		proto_decode (&cpb, &proto_chanlist_desc, &chanlist);
		if ((width = chanlist.bit_width) == 0)
			width = 16;
		mask = (1 << width) - 1;

		switch (chanlist.chan_type) {
		case CHAN_TYPE_8BIT_PORT:
			val = random() & 0xff;
			val = (val >> chanlist.bit_position) & mask;
			break;
		default:
			break;
		}
		proto_putbits (&dpb, val, width);
	}
}

int sock;
struct sockaddr_in hen_addr;

void rcv_soak (void);

double
get_secs (void)
{
	struct timeval tv;

	gettimeofday (&tv, NULL);
	return (tv.tv_sec + tv.tv_usec / 1e6);
}

void
usage (void)
{
	fprintf (stderr, "usage: chick\n");
	exit (1);
}

void
dump (void *buf, int n)
{
	int i;
	int j;
	int c;

	for (i = 0; i < n; i += 16) {
		printf ("%04x: ", i);
		for (j = 0; j < 16; j++) {
			if (i+j < n)
				printf ("%02x ", ((unsigned char *)buf)[i+j]);
			else
				printf ("   ");
		}
		printf ("  ");
		for (j = 0; j < 16; j++) {
			c = ((unsigned char *)buf)[i+j] & 0x7f;
			if (i+j >= n)
				putchar (' ');
			else if (c < ' ' || c == 0x7f)
				putchar ('.');
			else
				putchar (c);
		}
		printf ("\n");

	}
}

double
jitter (void)
{
	double r = (double)random () / RAND_MAX;

	return ((r - 0.5) * DATA_INTERVAL_SECS * 0.1);
}

void
maybe_xmit (void)
{
	unsigned char pkt[1000];
	int size;
	
	/* TODO: take into account generated jitter */
	if (get_secs () - last_xmit_ts < XMIT_INTERVAL_SECS)
		return;

	if (0) {
		dump (pkt, size);
		sendto (sock, pkt, size, 0,
			(struct sockaddr *)&hen_addr, sizeof hen_addr);
	}
	last_xmit_ts = get_secs ();
	
	listen_until_ts = last_xmit_ts + LISTEN_INTERVAL_SECS;
}

int
main (int argc, char **argv)
{
	int c;
	int port;
	struct timeval tv;
	fd_set rset;
	double now;
	double secs;
	char *key;
	
	key = NULL;
	while ((c = getopt (argc, argv, "vxk:")) != EOF) {
		switch (c) {
		case 'k':
			key = optarg;
			break;
		case 'x':
			xflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		default:
			usage ();
		}
	}

	if (optind != argc)
		usage ();

	if (get_my_mac_addr (my_mac_addr, MAC_LEN) < 0) {
		fprintf (stderr, "can't find my mac addr\n");
		exit (1);
	}
	printf ("my mac addr %02x:%02x:%02x:%02x:%02x:%02x\n",
		my_mac_addr[0], my_mac_addr[1], my_mac_addr[2], 
		my_mac_addr[3], my_mac_addr[4], my_mac_addr[5]);
	my_mac_hash = compute_mac_hash (my_mac_addr);
	printf ("my mac hash %d 0x%x\n", my_mac_hash, my_mac_hash);

	nvram_startup ();

	if (key != NULL) {
		int i, val;
		char *p;
		
		if (strlen (key) != HEN_KEY_LEN * 2) {
			fprintf (stderr, "invalid key\n");
			exit (1);
		}
		for (i = 0, p = key; i < HEN_KEY_LEN; i++, p += 2) {
			if (sscanf (p, "%2x", &val) != 1) {
				fprintf (stderr, "non hex digit in key\n");
				exit (1);
			}
			nvram.hen_key[i] = val;
		}

		nvram_save ();
		printf ("key saved\n");
		exit (0);
		
	}

	memcpy (chick_key, nvram.hen_key, HEN_KEY_LEN);
	memcpy (chick_key + HEN_KEY_LEN, my_mac_addr, MAC_LEN);

	if ((sock = setup_multicast (CHICK_HEN_MADDR, CHICK_HEN_PORT)) < 0) {
		fprintf (stderr, "can't setup multicast\n");
		exit (1);
	}

	port = CHICK_HEN_PORT;

	memset (&hen_addr, 0, sizeof hen_addr);
	hen_addr.sin_family = AF_INET;
	inet_aton (CHICK_HEN_MADDR, &hen_addr.sin_addr);
	hen_addr.sin_port = htons (port);

	printf ("sending to %s:%d\n", CHICK_HEN_MADDR, port);

	while (1) {
		rcv_soak ();
		
		if (xflag) {
			now = get_secs ();
			if (now >= next_data_ts) {
				collect_dpoint ();
				next_data_ts = now + DATA_INTERVAL_SECS 
					+ jitter ();
			}
			maybe_xmit ();
		}
		
		now = get_secs ();
		if (now < listen_until_ts) {
			secs = min (next_data_ts, listen_until_ts) - now;
			if (secs > 0) {
				tv.tv_sec = floor (secs);
				tv.tv_usec = floor ((secs - tv.tv_sec) * 1e6);
				FD_ZERO (&rset);
				FD_SET (sock, &rset);
				if (select(sock+1, &rset, NULL, NULL, &tv)<0) {
					perror ("select");
					exit (1);
				}
			}
		} else {
			secs = next_data_ts - now;
			if (secs > 0)
				usleep (secs * 1e6);
		}

	}

	return (0);
}

void
handle_probe (struct sockaddr_in *raddr, struct proto_hdr *rhdr, 
	      struct proto_buf *pb)
{
	unsigned char xbuf[10000];
	int xlen;
	struct proto_probe probe;
	struct proto_buf xpb;
	struct proto_hdr xhdr;
	struct proto_probe_response pr;
	
	printf ("probe\n");

	proto_decode (pb, &proto_probe_desc, &probe);

	if (probe.mac0 != my_mac_addr[0]
	    || probe.mac1 != my_mac_addr[1]
	    || probe.mac2 != my_mac_addr[2]
	    || probe.mac3 != my_mac_addr[3]
	    || probe.mac4 != my_mac_addr[4]
	    || probe.mac5 != my_mac_addr[5]) {
		printf ("probe: mac mismatch\n");
		return;
	}

	proto_encode_init (&xpb, xbuf, sizeof xbuf);
	xhdr.op = OP_PROBE_RESPONSE;
	proto_encode (&xpb, &proto_hdr_desc, &xhdr);
	proto_encode (&xpb, &proto_probe_response_desc, &pr);
	xlen = proto_used (&xpb);
	dump (xbuf, xlen);

	sendto (sock, xbuf, xlen, 0, (struct sockaddr *)raddr, sizeof *raddr);
}

void
handle_scan (struct sockaddr_in *raddr, struct proto_hdr *rhdr, 
	     struct proto_buf *pb)
{
	unsigned char xbuf[10000];
	int xlen;
	struct proto_scan scan;
	struct proto_buf xpb;
	struct proto_hdr xhdr;
	struct proto_probe_response pr;
	
	printf ("scan\n");

	proto_decode (pb, &proto_scan_desc, &scan);

	if (scan.divisor == 0) {
		printf ("ignore scan with zero divisor\n");
		return;
	}

	if (scan_digest (scan.key, scan.divisor, scan.remainder)) {
		xhdr.mac_hash = HEN_MAC_HASH;
		xhdr.op = OP_PROBE_RESPONSE;
		pr.mac0 = my_mac_addr[0];
		pr.mac1 = my_mac_addr[1];
		pr.mac2 = my_mac_addr[2];
		pr.mac3 = my_mac_addr[3];
		pr.mac4 = my_mac_addr[4];
		pr.mac5 = my_mac_addr[5];

		proto_encode_init (&xpb, xbuf, sizeof xbuf);
		proto_encode (&xpb, &proto_hdr_desc, &xhdr);
		proto_encode (&xpb, &proto_probe_response_desc, &pr);
		proto_sign (&xpb, nvram.hen_key, HEN_KEY_LEN);
		xlen = proto_used (&xpb);
		dump (xbuf, xlen);

		sendto (sock, xbuf, xlen, 0, 
			(struct sockaddr *)raddr, sizeof *raddr);
	}
}

void
handle_chanlist (struct sockaddr_in *raddr, struct proto_hdr *rhdr, 
		 struct proto_buf *pb)
{
	unsigned char xbuf[10000];
	int xlen;
	struct proto_chanlist chanlist;
	struct proto_buf xpb;
	struct proto_hdr xhdr;
	struct proto_ack ack;
	struct proto_cookie cookie_rpkt;
	struct proto_cookie cookie_xpkt;
	
	printf ("chanlist\n");

	proto_decode (pb, &proto_cookie_desc, &cookie_rpkt);

	proto_encode_init (&xpb, nvram.chanlist, sizeof nvram.chanlist);
	while (pb->used_bits + PROTO_CHANLIST_NBITS <= pb->avail_bits) {
		proto_decode (pb, &proto_chanlist_desc, &chanlist);
		proto_encode (&xpb, &proto_chanlist_desc, &chanlist);
	}
	nvram.chanlist_used = proto_used (&xpb);
	nvram_save ();
	nvram_startup ();

	proto_encode_init (&xpb, xbuf, sizeof xbuf);

	xhdr.mac_hash = HEN_MAC_HASH;
	xhdr.op = OP_ACK;
	cookie_xpkt.cookie = cookie_rpkt.cookie;
	ack.err = 0;

	proto_encode_init (&xpb, xbuf, sizeof xbuf);
	proto_encode (&xpb, &proto_hdr_desc, &xhdr);
	proto_encode (&xpb, &proto_cookie_desc, &cookie_xpkt);
	proto_encode (&xpb, &proto_ack_desc, &ack);
	proto_sign (&xpb, nvram.hen_key, HEN_KEY_LEN);
	xlen = proto_used (&xpb);
	printf ("ack\n");
	dump (xbuf, xlen);

	sendto (sock, xbuf, xlen, 0, 
		(struct sockaddr *)raddr, sizeof *raddr);
}

void
rcv_soak (void)
{
	struct sockaddr_in raddr;
	socklen_t raddr_len;
	unsigned char rbuf[10000];
	int len;
	struct proto_buf pb;
	struct proto_hdr hdr;

	raddr_len = sizeof raddr;

	while ((len = recvfrom (sock, rbuf, sizeof rbuf, 0,
				(struct sockaddr *)&raddr, &raddr_len)) >= 0) {
		if (vflag)
			dump (rbuf, len);

		if (rbuf[0] != BROADCAST_MAC_HASH && rbuf[0] != my_mac_hash) {
			printf ("skip mac hash 0x%x\n", rbuf[0]);
			continue;
		}
		
		proto_decode_init (&pb, rbuf, len);
		if (proto_checksig (&pb,
				    nvram.hen_key, HEN_KEY_LEN,
				    chick_key, CHICK_KEY_LEN) < 0) {
			printf ("bad sig\n");
			continue;
		}

		proto_decode (&pb, &proto_hdr_desc, &hdr);

		if (hdr.mac_hash != my_mac_hash
		    && hdr.mac_hash != BROADCAST_MAC_HASH) {
			printf ("not for me 0x%x\n", hdr.mac_hash);
			continue;
		}

		switch (hdr.op) {
		case OP_PROBE:
			handle_probe (&raddr, &hdr, &pb);
			break;
		case OP_SCAN:
			handle_scan (&raddr, &hdr, &pb);
			break;
		case OP_CHANLIST:
			handle_chanlist (&raddr, &hdr, &pb);
			break;
		default:
			printf ("unknown op %d 0x%x\n", hdr.op, hdr.op);
			dump (rbuf, len);
			break;
		}
	}
}

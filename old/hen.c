/* soak up data from multicast all local hosts: 224.0.0.1 */

#include "chick-hen.h"

void process_pkt (struct sockaddr_in *raddr, unsigned char *pkt, int len);


int vflag;

void
usage (void)
{
	fprintf (stderr, "usage: hen\n");
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

int
main (int argc, char **argv)
{
	int c;
	int sock;
	struct sockaddr_in raddr;
	socklen_t raddrlen;
	int len;
	unsigned char buf[10000];

	while ((c = getopt (argc, argv, "v")) != EOF) {
		switch (c) {
		case 'v':
			vflag = 1;
			break;
		default:
			usage ();
		}
	}

	if (optind != argc)
		usage ();

	if ((sock = setup_multicast (CHICK_HEN_MADDR, CHICK_HEN_PORT)) < 0) {
		fprintf (stderr, "can't setup multicast\n");
		exit (1);
	}

	printf ("listening on %s:%d\n", CHICK_HEN_MADDR, CHICK_HEN_PORT);

	fcntl (sock, F_SETFL, 0);

	while (1) {
		raddrlen = sizeof raddr;
		len = recvfrom (sock, buf, sizeof buf - 1, 0,
				(struct sockaddr *)&raddr, &raddrlen);
		if (len < 0) {
			perror ("recvfrom");
			exit (1);
		}
		buf[len] = 0;
		process_pkt (&raddr, buf, len);

		sendto (sock, "ok", 2, 0,
			(struct sockaddr *)&raddr, raddrlen);
	}

	return (0);
}

void
process_pkt (struct sockaddr_in *raddr, unsigned char *rawpkt, int len)
{
	struct chick_hen_data msg;
	int dat_avail;
	struct wirebuf wb;
	
	memset (&msg, 0, sizeof msg);
	
	init_wirebuf (&wb, rawpkt, len);
	msg.to_nodenum = get8 (&wb);
	msg.from_nodenum = get8 (&wb);
	msg.last_strength = get8 (&wb);
	msg.seq = get24 (&wb);
	
	dat_avail = wb.avail - wb.used - 4;

	wb.used += dat_avail;
	msg.hmac = get32 (&wb);
	
	printf ("%s:%d %d->%d str%x seq%x hmac%x avail%d\n", 
		inet_ntoa (raddr->sin_addr), ntohs (raddr->sin_port),
		msg.to_nodenum, msg.from_nodenum,
		msg.last_strength,
		msg.seq, msg.hmac,
		dat_avail);
	dump (rawpkt, len);
}

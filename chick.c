#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <fcntl.h>
#include <math.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

#include "chick-hen.h"

#define min(a,b) ((a)<(b)?(a):(b))

int vflag;

#define DATA_INTERVAL_SECS 0.1
#define XMIT_INTERVAL_SECS 1
#define LISTEN_INTERVAL_SECS 0.5

double last_xmit_ts;
double listen_until_ts;
double next_data_ts;




#define MAX_XMIT_DPOINTS 3

int sock;
struct sockaddr_in hen_addr;

int hen_nodenum;
int my_nodenum;
int last_rcv_strength;
uint32_t series_number;

uint32_t next_seq;

struct dpoint {
	uint32_t seq;
	double ts;
	double val;
};

#define NUM_DPOINTS 1000
struct dpoint dpoints[NUM_DPOINTS];
int dpoint_in, dpoint_out;

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

int
dpoint_avail (void)
{
	return ((dpoint_in - dpoint_out + NUM_DPOINTS) % NUM_DPOINTS);
}

struct dpoint *
peek_dpoint (int offset)
{
	if (offset >= dpoint_avail ())
		return (NULL);
	
	return (&dpoints[(dpoint_out + offset) % NUM_DPOINTS]);
}

void
consume_dpoint (void)
{
	dpoint_out = (dpoint_out + 1) % NUM_DPOINTS;
}

struct dpoint *
alloc_dpoint (void)
{
	if ((dpoint_in + 1) % NUM_DPOINTS == dpoint_out)
		consume_dpoint ();

	return (&dpoints[dpoint_in]);
}

void
save_dpoint (struct dpoint *dp)
{
	dpoint_in = (dpoint_in + 1) % NUM_DPOINTS;
}

void
dump_data (void)
{
	int idx;
	struct dpoint *dp;
	FILE *outf;
	double first_ts;

	if ((outf = fopen ("x.dat", "w")) == NULL) {
		fprintf (stderr, "can't create x.dat\n");
		exit (1);
	}

	first_ts = 0;
	for (idx = dpoint_out; 
	     idx != dpoint_in; 
	     idx = (idx + 1) % NUM_DPOINTS) {
		dp = &dpoints[idx];
		if (first_ts == 0)
			first_ts = dp->ts;
		fprintf (outf, "%g %g\n", dp->ts - first_ts, dp->val);
	}
	fclose (outf);
}

double
jitter (void)
{
	double r = (double)random () / RAND_MAX;

	return ((r - 0.5) * DATA_INTERVAL_SECS * 0.1);
}

void
collect_data (void)
{
	struct dpoint *dp;
	double now;
	
	now = get_secs ();
	if (now < next_data_ts)
		return;

	printf ("get data\n");

	if ((dp = alloc_dpoint ()) != NULL) {
		dp->seq = next_seq++;
		dp->ts = get_secs ();
		dp->val = sin (dp->ts * 0.1 * 2 * M_PI);
		save_dpoint (dp);
	}

	dump_data();

	next_data_ts = now + DATA_INTERVAL_SECS + jitter ();
}

void
put8 (unsigned char **pp, int val)
{
	*(*pp)++ = val;
}

void
put24 (unsigned char **pp, int val)
{
	*(*pp)++ = val;
	*(*pp)++ = val >> 8;
	*(*pp)++ = val >> 16;
}

void
put32 (unsigned char **pp, int val)
{
	*(*pp)++ = val;
	*(*pp)++ = val >> 8;
	*(*pp)++ = val >> 16;
	*(*pp)++ = val >> 24;
}

void
put_double (unsigned char **pp, double val)
{
	memcpy (*pp, &val, sizeof val);
	(*pp) += sizeof val;
}

void
maybe_xmit (void)
{
	int thistime;
	struct dpoint *first, *dp;
	unsigned char *p;
	unsigned char pkt[1000];
	int off;
	uint32_t hmac;
	int size;
	
	/* TODO: take into account generated jitter */
	if (get_secs () - last_xmit_ts < XMIT_INTERVAL_SECS)
		return;

	if ((thistime = dpoint_avail ()) == 0)
		return;
	
	if (thistime > MAX_XMIT_DPOINTS)
		thistime = MAX_XMIT_DPOINTS;

	first = peek_dpoint (0);

	p = pkt;
	put8 (&p, hen_nodenum);
	put8 (&p, my_nodenum);
	put8 (&p, last_rcv_strength);
	put24 (&p, first->seq);

	for (off = 0; off < thistime; off++) {
		if ((dp = peek_dpoint (off)) == NULL)
			break;
		put_double (&p, dp->ts);
		put_double (&p, dp->val);
	}
	
	hmac = series_number;
	
	put32 (&p, hmac);
	
	size = p - pkt;
	
	dump (pkt, size);
	sendto (sock, pkt, size, 0,
		(struct sockaddr *)&hen_addr, sizeof hen_addr);
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

	port = CHICK_HEN_PORT;

	sock = socket (AF_INET, SOCK_DGRAM, 0);

	fcntl (sock, F_SETFL, O_NONBLOCK);

	memset (&hen_addr, 0, sizeof hen_addr);
	hen_addr.sin_family = AF_INET;
	inet_aton (HEN_ADDR, &hen_addr.sin_addr);
	hen_addr.sin_port = htons (port);

	printf ("sending to %s:%d\n", HEN_ADDR, port);

	while (1) {
		rcv_soak ();
		
		collect_data ();
		maybe_xmit ();

		now = get_secs ();
		if (listen_until_ts > now) {
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
rcv_soak (void)
{
	struct sockaddr_in raddr;
	socklen_t raddr_len;
	char buf[10000];
	int len;

	raddr_len = sizeof raddr;

	while ((len = recvfrom (sock, buf, sizeof buf, 0,
				(struct sockaddr *)&raddr, &raddr_len)) >= 0) {
		dump (buf, len);
	}
}

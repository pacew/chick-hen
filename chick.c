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

#define DATA_INTERVAL 0.05 /* secs */

double
get_secs (void)
{
	struct timeval tv;

	gettimeofday (&tv, NULL);
	return (tv.tv_sec + tv.tv_usec / 1e6);
}

struct timeout {
	struct timeout *next;
	double due;
	void (*func)(void *arg);
	void *arg;
};

struct timeout *timeouts;

void
set_timeout (double delta_secs, void (*func)(void *arg), void *arg)
{
	struct timeout *tp, **otp;

	tp = calloc (1, sizeof *tp);
	tp->due = get_secs () + delta_secs;
	tp->func = func;
	tp->arg = arg;

	for (otp = &timeouts; *otp; otp = &(*otp)->next) {
		if (tp->due < (*otp)->due)
			break;
	}

	tp->next = *otp;
	*otp = tp;
}

double
run_timeouts (void)
{
	double now;
	struct timeout *tp;

	now = get_secs ();
	while ((tp = timeouts) != NULL) {
		if (now < tp->due)
			return (tp->due - now);

		(*tp->func)(tp->arg);
		timeouts = tp->next;
		free (tp);
	}

	return (-1);
}

int vflag;

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

struct dpoint {
	double ts;
	double val;
};

#define NUM_DPOINTS 1000
struct dpoint dpoints[NUM_DPOINTS];
int dpoint_in, dpoint_out;

struct dpoint *
peek_dpoint (void)
{
	if (dpoint_in == dpoint_out)
		return (NULL);
	return (&dpoints[dpoint_out]);
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

void
collect_data (void *arg)
{
	struct dpoint *dp;
	
	printf ("tick\n");

	if ((dp = alloc_dpoint ()) != NULL) {
		dp->ts = get_secs ();
		dp->val = sin (dp->ts * 0.1 * 2 * M_PI);
		save_dpoint (dp);
	}

	dump_data();

	set_timeout (DATA_INTERVAL, collect_data, NULL);
}


void do_rcv (void);

int sock;

int
main (int argc, char **argv)
{
	int c;
	struct sockaddr_in addr;
	int port;
	struct timeval tv;
	fd_set rset;
	double delta;

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

	memset (&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	inet_aton (HEN_ADDR, &addr.sin_addr);
	addr.sin_port = htons (port);

	printf ("sending to %s:%d\n", HEN_ADDR, port);

	set_timeout (DATA_INTERVAL, collect_data, NULL);

	while (1) {
		do_rcv ();

		delta = run_timeouts ();
		if (delta > 0) {
			tv.tv_sec = floor (delta);
			tv.tv_usec = floor ((delta - tv.tv_sec) * 1e6);
			FD_ZERO (&rset);
			FD_SET (sock, &rset);
			if (select (sock + 1, &rset, NULL, NULL, &tv) < 0) {
				perror ("select");
				exit (1);
			}
		}
	}

	return (0);
}

void
do_rcv (void)
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

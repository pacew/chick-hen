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

double
get_secs (void)
{
	struct timeval tv;

	gettimeofday (&tv, NULL);
	return (tv.tv_sec + tv.tv_usec / 1e6);
}

void do_rcv (void);

int sock;

int
main (int argc, char **argv)
{
	int c;
	struct sockaddr_in addr;
	char buf[10000];
	int port;
	double next_xmit, now, delta;
	struct timeval tv;
	fd_set rset;

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

	next_xmit = get_secs ();

	while (1) {
		now = get_secs ();
		delta = next_xmit - now;
		if (delta <= 0) {
			strcpy (buf, "hello");
			sendto (sock, buf, strlen (buf), 0,
				(struct sockaddr *)&addr, sizeof addr);
			next_xmit = now + 1;
		} else {
			/* delta > 0 */
			tv.tv_sec = floor (delta);
			tv.tv_usec = floor ((delta - tv.tv_sec) * 1e6);
			FD_ZERO (&rset);
			FD_SET (sock, &rset);
			if (select (sock + 1, &rset, NULL, NULL, &tv) < 0) {
				perror ("select");
				exit (1);
			}
			if (FD_ISSET (sock, &rset))
				do_rcv ();
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

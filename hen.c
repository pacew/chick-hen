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
	struct sockaddr_in addr, raddr;
	socklen_t raddrlen;
	int len;
	unsigned char buf[10000];
	int port;
	struct ifaddrs *ifaddr, *ifa;
	struct ip_mreq group;

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

	int reuse = 1;
	if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR,
			&reuse, sizeof reuse) < 0) {
		perror ("SO_REUSEADDR");
		exit (1);
	}


	memset (&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	inet_aton (HEN_ADDR, &addr.sin_addr);
	addr.sin_port = htons (port);

	if (getifaddrs (&ifaddr) < 0) {
		perror ("getifaddrs");
		exit (1);
	}
	
	for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family == AF_INET) {
			struct sockaddr_in *iaddr;
			iaddr = (struct sockaddr_in *)ifa->ifa_addr;
			memset (&group, 0, sizeof group);
			group.imr_multiaddr = addr.sin_addr;
			group.imr_interface = iaddr->sin_addr;
			if (setsockopt (sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
					&group, sizeof group) < 0) {
				perror ("add membership");
				exit (1);
			}
		}
	}

	if (bind (sock, (struct sockaddr *)&addr, sizeof addr) < 0) {
		perror ("bind");
		exit (1);
	}

	printf ("listening on %s:%d\n", HEN_ADDR, port);

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

	msg.hmac = get32 (&wb);
	
	printf ("%s:%d %d->%d s%d seq%d hmac%d avail%d\n", 
		inet_ntoa (raddr->sin_addr), ntohs (raddr->sin_port),
		msg.to_nodenum, msg.from_nodenum,
		msg.last_strength,
		msg.seq, msg.hmac,
		dat_avail);
	dump (rawpkt, len);
}

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <memory.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>


#include "multicast.h"

int
setup_multicast (char *addr_str, int port)
{
	int sock;
	struct sockaddr_in maddr;
	struct ifaddrs *ifaddr, *ifa;
	
	if ((sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror ("socket");
		goto bad;
	}

	if (fcntl (sock, F_SETFL, O_NONBLOCK) < 0) {
		perror ("nonblock");
		goto bad;
	}

	int reuse = 1;
	if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR,
			&reuse, sizeof reuse) < 0) {
		perror ("SO_REUSEADDR");		
		goto bad;
	}

	int loop = 1;
	if (setsockopt (sock, IPPROTO_IP, IP_MULTICAST_LOOP, 
			&loop, sizeof loop) < 0) {
		perror ("turn off loopback");
		goto bad;
	}

	memset (&maddr, 0, sizeof maddr);
	maddr.sin_family = AF_INET;
	inet_aton (addr_str, &maddr.sin_addr);
	maddr.sin_port = htons (port);

	if (getifaddrs (&ifaddr) < 0) {
		perror ("getifaddrs");
		goto bad;
	}
	
	for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family == AF_INET) {
			struct sockaddr_in *iaddr 
				= (struct sockaddr_in *)ifa->ifa_addr;
			struct ip_mreq group;
			memset (&group, 0, sizeof group);
			group.imr_multiaddr = maddr.sin_addr;
			group.imr_interface = iaddr->sin_addr;
			if (setsockopt (sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
					&group, sizeof group) < 0) {
				perror ("add membership");
				goto bad;
			}
		}
	}

	if (bind (sock, (struct sockaddr *)&maddr, sizeof maddr) < 0) {
		perror ("bind");
		goto bad;
	}

	return (sock);

bad:
	if (sock >= 0)
		close (sock);

	return (-1);
}

int
get_my_mac_addr (unsigned char *mac, int mac_len)
{
	struct ifaddrs *ifaddr, *ifa;
	struct sockaddr_ll *laddr;
	int i;

	if (getifaddrs (&ifaddr) >= 0) {
		for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr->sa_family != AF_PACKET)
				continue;
			
			laddr = (struct sockaddr_ll *)ifa->ifa_addr;
			
			if (laddr->sll_halen != mac_len)
				continue;

			/* ignore all zero macs */
			for (i = 0; i < mac_len; i++) {
				if (laddr->sll_addr[i] != 0)
					break;
			}
			if (i == mac_len)
				continue;

			memcpy (mac, laddr->sll_addr, mac_len);
			return (0);
		}
	}

	return (-1);
}

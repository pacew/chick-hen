#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <math.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define HEN_ADDR "224.0.0.130"
#define CHICK_HEN_PORT 32519

struct wirebuf {
	void *base;
	int avail;
	int used;
};

void init_wirebuf (struct wirebuf *wb, void *base, int avail);

void put8 (struct wirebuf *wb, int val);
void put24 (struct wirebuf *wb, int val);
void put32 (struct wirebuf *wb, int val);
void put_double (struct wirebuf *wb, double val);

int get8 (struct wirebuf *wb);
int get24 (struct wirebuf *wb);
uint32_t get32 (struct wirebuf *wb);
double get_double (struct wirebuf *wb);


/* define pkt fields, but not wire format */
struct chick_hen_data {
	uint8_t to_nodenum;
	uint8_t from_nodenum;
	uint8_t last_strength;
	uint32_t seq; /* 20 bits */
	uint32_t hmac; /* 24 bits */
	void *data;
	int data_len;
};

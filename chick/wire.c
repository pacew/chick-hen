#include "chick-hen.h"

/* https://docs.python.org/2/library/struct.html */
/* https://nodemcu.readthedocs.io/en/master/en/modules/struct/ */

void
init_wirebuf (struct wirebuf *wb, void *base, int avail)
{
	wb->base = base;
	wb->avail = avail;
	wb->used = 0;
}


void
put8 (struct wirebuf *wb, int val)
{
	if (wb->used < wb->avail) {
		unsigned char *p = wb->base + wb->used;
		*p = val;
		wb->used++;
	}
}

void
put24 (struct wirebuf *wb, int val)
{
	put8 (wb, val);
	put8 (wb, val >> 8);
	put8 (wb, val >> 16);
}

void
put32 (struct wirebuf *wb, int val)
{
	put8 (wb, val);
	put8 (wb, val >> 8);
	put8 (wb, val >> 16);
	put8 (wb, val >> 24);
}

void
put_double (struct wirebuf *wb, double val)
{
	unsigned char *p = (unsigned char *)&val;
	int i;
	
	for (i = 0; i < sizeof val; i++)
		put8 (wb, p[i]);
}

int
get8 (struct wirebuf *wb)
{
	if (wb->used < wb->avail)
		return (((unsigned char *)wb->base)[wb->used++]);
	return (0);
}

int
get24 (struct wirebuf *wb)
{
	uint8_t b0, b1, b2;
	b0 = get8 (wb);
	b1 = get8 (wb);
	b2 = get8 (wb);
	return ((b2 << 16) | (b1 << 8) | b0);
}

uint32_t
get32 (struct wirebuf *wb)
{
	uint8_t b0, b1, b2, b3;
	b0 = get8 (wb);
	b1 = get8 (wb);
	b2 = get8 (wb);
	b3 = get8 (wb);
	return ((b3 << 24) | (b2 << 16) | (b1 << 8) | b0);
}

double
get_double (struct wirebuf *wb)
{
	double val;
	unsigned char *p;
	int i;

	p = (unsigned char *)&val;
	for (i = 0; i < sizeof val; i++) {
		*p++ = get8 (wb);
	}
	return (val);
}


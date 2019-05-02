#include <stdio.h>
#include <stdint.h>
#include <memory.h>

#include "proto-gen.h"

/* wire format is little endian, regardless of host byte order */

/* get value from structure, so host byte order */
static uint32_t
getval (struct field_desc *fp, void *cval)
{
	uint16_t val16;
	uint32_t val32;

	if (fp->bits <= 8)
		return (*(uint8_t *)(cval + fp->offset));
	if (fp->bits <= 16) {
		memcpy (&val16, cval + fp->offset, sizeof val16);
		return (val16);
	}

	memcpy (&val32, cval + fp->offset, sizeof val32);
	return (val32);
}

/* put value into structure, so host byte order */
static void
putval (struct field_desc *fp, void *cval, uint32_t val)
{
	
	if (fp->bits <= 8) {
		*(uint8_t *)(cval + fp->offset) = val;
	} else if (fp->bits <= 16) {
		uint16_t val16 = val;
		memcpy (cval + fp->offset, &val16, sizeof val16);
	} else {
		memcpy (cval + fp->offset, &val, sizeof val);
	}
}

void
proto_print (FILE *f, struct proto_desc *desc, void *cval)
{
	int fieldnum;
	struct field_desc *fp;
	uint32_t val;
	
	printf ("pkt %s\n", desc->name);
	for (fieldnum = 0, fp = desc->fields; 
	     fieldnum < desc->nfields; 
	     fieldnum++, fp++) {
		val = getval (fp, cval);
		
		printf ("  %s %d (0x%x)\n", fp->name, val, val);
	}
}

int
proto_putbits (struct proto_buf *pb, uint32_t val, int bits)
{
	int togo, thistime;
	uint32_t remaining_val;
	int byte_off, bit_off;
	uint8_t *loc;
	uint16_t mask;
	
	togo = bits;
	remaining_val = val;
	while (togo > 0) {
		byte_off = pb->used_bits / 8;
		bit_off = pb->used_bits & 7;
		thistime = 8 - bit_off;
		if (thistime > togo)
			thistime = togo;

		if (pb->used_bits + thistime > pb->avail_bits)
			return (-1);

		loc = pb->base + byte_off;
		mask = (1 << thistime) - 1;

		if (bit_off == 0)
			*loc = 0;
		*loc |= (remaining_val & mask) << bit_off;

		pb->used_bits += thistime;
		togo -= thistime;
		remaining_val >>= thistime;
	}

	return (0);
}

int
proto_putbytes (struct proto_buf *pb, void *buf, int len)
{
	int used;
	
	used = proto_used (pb);
	if ((used + len) * 8 > pb->avail_bits)
		return (-1);
	memcpy (pb->base + used, buf, len);
	pb->used_bits += len * 8;

	return (0);
}


static uint32_t
decode_val (struct proto_buf *pb, int bits)
{
	int togo;
	uint32_t output;
	int output_shift;
	int byte_off, bit_off;
	int thistime;
	int mask;
	uint8_t *loc;
	int nibble;

	togo = bits;
	output = 0;
	output_shift = 0;
	while (togo > 0) {
		byte_off = pb->used_bits / 8;
		bit_off = pb->used_bits & 7;
		thistime = 8 - bit_off;
		if (thistime > togo)
			thistime = togo;
		if (pb->used_bits + thistime > pb->avail_bits)
			break;

		mask = (1 << thistime) - 1;

		loc = pb->base + byte_off;
		nibble = (*loc >> bit_off) & mask;
		output |= (nibble << output_shift);
		
		pb->used_bits += thistime;
		togo -= thistime;
		output_shift += thistime;
	}

	return (output);
}

void
proto_encode_init (struct proto_buf *pb, void *buf, int size)
{
	memset (pb, 0, sizeof *pb);
	pb->base = buf;
	pb->avail_bits = size * 8;
}

void
proto_decode_init (struct proto_buf *pb, void *buf, int full_size)
{
	memset (pb, 0, sizeof *pb);
	pb->base = buf;
	pb->avail_bits = full_size * 8;
}

/* strip sig from end of pkt. return 0 if valid sig, else -1 */
int
proto_checksig (struct proto_buf *pb, 
		void *hen_key, int hen_key_len,
		void *chick_key, int chick_key_len)
{
	unsigned char *pkt_sig;
	unsigned char computed_sig[PKT_SIG_SIZE];
	unsigned char *key;
	int key_len;
	int payload_nbytes;
	
	/* make sure there's at least a mac_hash first byte and a sig at end */
	if (pb->avail_bits < (1 + PKT_SIG_SIZE) * 8)
		return (-1);
	
	payload_nbytes = pb->avail_bits / 8 - PKT_SIG_SIZE;
	
	pkt_sig = pb->base + payload_nbytes;
	pb->avail_bits = payload_nbytes * 8;

	if (*(unsigned char *)pb->base == BROADCAST_MAC_HASH) {
		key = hen_key;
		key_len = hen_key_len;
	} else {
		key = chick_key;
		key_len = chick_key_len;
	}
	compute_pkt_sig (computed_sig, key, key_len, pb->base, payload_nbytes);

	if (memcmp (computed_sig, pkt_sig, PKT_SIG_SIZE) == 0)
		return (0);

	return (-1);
}

/* returns -1 on output overflow, number of bytes occupied by output */
void
proto_encode (struct proto_buf *pb, struct proto_desc *desc, void *cval)
{
	int fieldnum;
	struct field_desc *fp;
	uint32_t val;
	
	for (fieldnum = 0, fp = desc->fields; 
	     fieldnum < desc->nfields; 
	     fieldnum++, fp++) {
		if (pb->used_bits + fp->bits > pb->avail_bits) {
			pb->err = 1;
			break;
		}
		val = getval (fp, cval);
		if (proto_putbits (pb, val, fp->bits) < 0) {
			pb->err = 1;
			break;
		}
	}
}

void
proto_sign (struct proto_buf *pb, void *key, int keylen)
{
	int used_bytes;
	
	used_bytes = proto_used (pb);
	if ((used_bytes + PKT_SIG_SIZE) * 8 > pb->avail_bits) {
		pb->err = 1;
		return;
	}
	compute_pkt_sig (pb->base + used_bytes, 
			 key, keylen,
			 pb->base, used_bytes);
	pb->used_bits += PKT_SIG_SIZE * 8;
}

int
proto_used (struct proto_buf *pb)
{
	if (pb->err)
		return (-1);
	return ((pb->used_bits + 7) / 8);
}

/* if input is too short, trailing fields are set to 0 */
void
proto_decode (struct proto_buf *pb, struct proto_desc *desc, void *cval)
{
	int fieldnum;
	struct field_desc *fp;
	uint32_t val;
	
	memset (cval, 0, desc->size);
	for (fieldnum = 0, fp = desc->fields; 
	     fieldnum < desc->nfields; 
	     fieldnum++, fp++) {
		val = decode_val (pb, fp->bits);
		putval (fp, cval, val);
	}
}

#include <stdio.h>
#include <stdint.h>
#include <memory.h>

#include "proto-gen.h"

/* encoded format is little endian, regardless of host byte order */

static uint32_t
getval (struct field_desc *fp, void *cval)
{
	if (fp->bits <= 8)
		return (*(uint8_t *)(cval + fp->offset));
	if (fp->bits <= 16)
		return (*(uint16_t *)(cval + fp->offset));
	return (*(uint32_t *)(cval + fp->offset));
}

static void
putval (struct field_desc *fp, void *cval, uint32_t val)
{
	if (fp->bits <= 8)
		*(uint8_t *)(cval + fp->offset) = val;
	else if (fp->bits <= 16)
		*(uint16_t *)(cval + fp->offset) = val;
	else
		*(uint32_t *)(cval + fp->offset) = val;
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

static int
encode_val (struct proto_buf *pb, uint32_t val, int bits)
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
proto_init (struct proto_buf *pb, void *buf, int size)
{
	pb->base = buf;
	pb->used_bits = 0;
	pb->avail_bits = size * 8;
	pb->err = 0;
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
		if (encode_val (pb, val, fp->bits) < 0) {
			pb->err = 1;
			break;
		}
	}
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

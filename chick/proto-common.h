#ifndef _PROTO_H_
#define _PROTO_H_

struct field_desc {
	char *name;
	int bits;
	int offset;
};

struct proto_desc {
	char *name;
	int nfields;
	struct field_desc *fields;
	int size;
};

struct proto_buf {
	int err;
	void *base;
	int used_bits;
	int avail_bits;
	int sig_ok;
};

struct proto_digest {
	unsigned char digest[4];
};

unsigned char *my_mac_addr;

void proto_decode_init (struct proto_buf *pb, void *buf, int size);
void proto_encode_init (struct proto_buf *pb, void *buf, int size);
void proto_print (FILE *f, struct proto_desc *desc, void *cval);
void proto_encode (struct proto_buf *pb, struct proto_desc *desc, void *cval);
void proto_sign (struct proto_buf *pb);
int proto_used (struct proto_buf *pb);
void proto_decode (struct proto_buf *pb, struct proto_desc *desc, void *cval);

void compute_digest (struct proto_digest *dp, void *buf, int len);
int scan_digest (uint8_t key, uint8_t divisor, uint8_t remainder);
uint32_t simple_digest (void *buf, int size);




#endif /* _PROTO_H_ */

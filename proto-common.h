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
	void *base;
	int used_bits;
	int avail_bits;
};

void proto_init (struct proto_buf *pb, void *buf, int size);
void proto_print (FILE *f, struct proto_desc *desc, void *cval);
int proto_encode (struct proto_buf *pb, struct proto_desc *desc, void *cval);
void proto_decode (struct proto_buf *pb, struct proto_desc *desc, void *cval);



#define CHICK_HEN_VERS 1

#define PKT_TO_NODENUM 0
#define PKT_FROM_NODENUM 1
#define PKT_OP 2
#define PKT_PAYLOAD 3

#define BROADCAST_NODENUM 99

#define OP_PROBE 0x40
#define OP_PROBE_RESPONSE 0x41

#endif /* _PROTO_H_ */

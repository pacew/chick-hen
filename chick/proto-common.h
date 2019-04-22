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

#define PKT_SIG_SIZE 4

#define HEN_KEY_LEN 32
#define MAC_LEN 6
#define CHICK_KEY_LEN (HEN_KEY_LEN + MAC_LEN)
unsigned char chick_key[CHICK_KEY_LEN];

unsigned char my_mac_addr[MAC_LEN];

void proto_decode_init (struct proto_buf *pb, 
			void *hen_key, int hen_key_len,
			void *chick_key, int chick_key_len,
			void *buf, int size);
void proto_encode_init (struct proto_buf *pb, void *buf, int size);
void proto_print (FILE *f, struct proto_desc *desc, void *cval);
void proto_encode (struct proto_buf *pb, struct proto_desc *desc, void *cval);
void proto_sign (struct proto_buf *pb, void *key, int keylen);
int proto_used (struct proto_buf *pb);
void proto_decode (struct proto_buf *pb, struct proto_desc *desc, void *cval);

void digest_init (void);
void compute_pkt_sig (unsigned char *sig, 
		      void *key, int keylen, 
		      void *buf, int len);
int scan_digest (uint8_t key, uint8_t divisor, uint8_t remainder);
uint32_t simple_digest (void *buf, int size);
int compute_mac_hash (unsigned char *mac_bin);

#endif /* _PROTO_H_ */

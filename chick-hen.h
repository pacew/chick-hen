#define HEN_ADDR "224.0.0.130"
#define CHICK_HEN_PORT 32519

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

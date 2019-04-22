#include <stdint.h>
#include <memory.h>
/* apt-get install libssl-dev */
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

#include "proto-gen.h"

void
compute_pkt_sig (unsigned char *sig, void *key, int keylen, void *buf, int len)
{
	HMAC_CTX *ctx;
	unsigned char full_digest[EVP_MAX_MD_SIZE];
	
	ctx = HMAC_CTX_new();

	HMAC_Init_ex(ctx,
		     key, keylen,
		     EVP_sha256(), NULL);
	
	HMAC_Update (ctx, buf, len);

	HMAC_Final(ctx, full_digest, NULL);

	memcpy (sig, full_digest, PKT_SIG_SIZE);

	HMAC_CTX_free(ctx);
}

/*
 * hash key using my mac address as the key. divide the first byte
 * by the divisor and return true if the remainder matches
 */
int
scan_digest (uint8_t key, uint8_t divisor, uint8_t remainder)
{
	unsigned char full_digest[EVP_MAX_MD_SIZE];
	unsigned char keybuf[1];

	keybuf[0] = key;

	HMAC(EVP_sha256(),
	     keybuf, 1,
	     my_mac_addr, 6,
	     full_digest, NULL);

	printf ("scan %d %d %d = %d\n",
		full_digest[0], divisor, remainder,
		full_digest[0] % divisor);

	if (full_digest[0] % divisor == remainder)
		return (1);

	return (0);
}

/* first 31 bits of sha256 of buffer as a non-neg integer (little endian) */
uint32_t
simple_digest (void *buf, int len)
{
	unsigned char full_digest[SHA256_DIGEST_LENGTH];
	uint32_t val;

	SHA256(buf, len, full_digest);

	val = full_digest[0] 
		| (full_digest[1] << 8) 
		| (full_digest[2] << 16)
		| ((full_digest[3] & 0x7f) << 24);
	return (val);
}

int
compute_mac_hash (unsigned char *mac_bin)
{
	int mac_hash;
	
	mac_hash = simple_digest (mac_bin, 6) & 0xff;
	switch (mac_hash) {
	case BROADCAST_MAC_HASH:
	case HEN_MAC_HASH:
	case 0:
	case 0xff:
		mac_hash = SUBSTITUTE_MAC_HASH;
	}
	return (mac_hash);
}
	

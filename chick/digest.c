#include <stdint.h>
#include <memory.h>
/* apt-get install libssl-dev */
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "proto-common.h"

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

uint32_t
simple_digest (void *buf, int len)
{
	unsigned char full_digest[EVP_MAX_MD_SIZE];
	uint32_t val;

	/* HMAC chokes if key is NULL, even if key len is 0 */
	HMAC(EVP_sha256(),
	     "", 0,
	     buf, len,
	     full_digest, NULL);

	memcpy (&val, full_digest, sizeof val);
	return (val);
}


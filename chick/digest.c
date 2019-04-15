#include <stdint.h>
#include <memory.h>
/* apt-get install libssl-dev */
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "proto-common.h"

static unsigned char system_key[32];
static int digest_ready;

void
digest_init (void)
{
	FILE *f;
	int i;
	int val;

	if (digest_ready)
		return;

	if ((f = fopen (".system_key", "r")) == NULL) {
		fprintf (stderr, "can't open .system_key\n");
		exit (1);
	}
	
	for (i = 0; i < sizeof system_key; i++) {
		if (fscanf (f, "%2x", &val) != 1) {
			fprintf (stderr, "bad .system_key\n");
			exit (1);
		}
		system_key[i] = val;
	}

	fclose (f);

	digest_ready = 1;
}

void
compute_digest (struct proto_digest *dp, void *buf, int len)
{
	unsigned char full_digest[EVP_MAX_MD_SIZE];
	
	digest_init ();

	HMAC(EVP_sha256(), 
	     system_key, sizeof system_key,
	     buf, len,
	     full_digest, NULL);

	memcpy (dp->digest, full_digest, sizeof dp->digest);
}

int
scan_digest (uint8_t key, uint8_t divisor, uint8_t remainder)
{
	unsigned char full_digest[EVP_MAX_MD_SIZE];

	HMAC(EVP_sha256(),
	     &key, 1,
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


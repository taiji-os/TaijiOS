/* Stub implementations for crypto functions not needed for Dis VM debugging */

#include "lib9.h"
#include "interp.h"

void keyringmodinit(void) {}
void cryptmodinit(void) {}
void ipintsmodinit(void) {}

void* TIPint = H;  /* Stub type reference */

/* Stub hmac_sha1 for devcap.c */
struct DigestState;  /* Forward declaration */
typedef struct DigestState DigestState;

DigestState* hmac_sha1(uchar *p, u32 len, uchar *key, u32 klen, uchar *digest, DigestState *s) {
	/* No-op stub */
	if(digest) memset(digest, 0, 20);
	return H;
}


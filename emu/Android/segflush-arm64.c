/*
 * ARM64 cache flush for Android
 * Uses __builtin___clear_cache for Android compatibility
 */

#include <sys/types.h>
#include <stdint.h>

#include "dat.h"

/*
 * segflush: flush instruction cache for ARM64 on Android
 * Android's LLVM compiler supports __builtin___clear_cache
 */
int
segflush(void *a, ulong n)
{
	if(n != 0) {
		/* Clear cache from a to a+n */
		__builtin___clear_cache((char*)a, (char*)a + n);
	}
	return 0;
}

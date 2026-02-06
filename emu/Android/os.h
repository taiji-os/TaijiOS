/*
 * os.h - Android-specific function declarations for libsec and libmp
 */

#include "lib9.h"

/* Random number generation functions */
extern void genrandom(uchar *p, int n);
extern ulong truerand(void);
extern vlong nsec(void);

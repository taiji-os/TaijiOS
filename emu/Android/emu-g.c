#include "dat.h"
#include "fns.h"
#include "error.h"
#include "interp.h"

#include "emu-g.root.h"

#ifndef KERNDATE
#define KERNDATE 1729046400UL  /* 2024-10-15 default build date */
#endif

/*
 * Note: devtab, ndevs, links, and modinit are defined in emu.c
 * This file contains only the Android-specific globals
 */

	void setpointer(int x, int y){USED(x); USED(y);}
	/* strtochan is defined in libdraw/chan.c */
char* conffile = "emu-g";
ulong kerndate = KERNDATE;

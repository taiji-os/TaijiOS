#include "dat.h"
#include "fns.h"
#include "error.h"
#include "interp.h"

#include "emu-g.root.h"

#ifndef KERNDATE
#define KERNDATE 1729046400UL  /* 2024-10-15 default build date */
#endif

ulong ndevs = 17;

extern Dev rootdevtab;
extern Dev consdevtab;
extern Dev envdevtab;
extern Dev mntdevtab;
extern Dev pipedevtab;
extern Dev progdevtab;
extern Dev profdevtab;
extern Dev srvdevtab;
extern Dev dupdevtab;
extern Dev ssldevtab;
extern Dev capdevtab;
extern Dev fsdevtab;
extern Dev cmddevtab;
extern Dev indirdevtab;
extern Dev ipdevtab;
extern Dev eiadevtab;
extern Dev memdevtab;
Dev* devtab[]={
	&rootdevtab,
	&consdevtab,
	&envdevtab,
	&mntdevtab,
	&pipedevtab,
	&progdevtab,
	&profdevtab,
	&srvdevtab,
	&dupdevtab,
	&ssldevtab,
	&capdevtab,
	&fsdevtab,
	&cmddevtab,
	&indirdevtab,
	&ipdevtab,
	&eiadevtab,
	&memdevtab,
	nil,
	nil,
	nil,
	nil,
	nil,
	nil,
	nil,
	nil,
	nil,
};

void links(void){
}

extern void sysmodinit(void);
extern void mathmodinit(void);
extern void srvmodinit(void);
extern void keyringmodinit(void);
extern void cryptmodinit(void);
extern void ipintsmodinit(void);
extern void loadermodinit(void);
void modinit(void){
	sysmodinit();
	mathmodinit();
	srvmodinit();
	keyringmodinit();
	cryptmodinit();
	ipintsmodinit();
	loadermodinit();
}

	void setpointer(int x, int y){USED(x); USED(y);}
	/* strtochan is defined in libdraw/chan.c */
char* conffile = "emu-g";
ulong kerndate = KERNDATE;

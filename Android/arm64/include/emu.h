/*
 * Android ARM64 system- and machine-specific declarations for emu:
 * floating-point save and restore, signal handling primitive, and
 * implementation of the current-process variable `up'.
 */

/*
 * This structure must agree with FPsave and FPrestore asm routines
 */
typedef struct FPU FPU;
struct FPU
{
	uchar	env[528];	/* 32 Q regs (16 bytes each) + FPCR + FPSR */
};

/*
 * Android uses pthreads, so we need a different approach
 * The Proc structure will have a pthread-specific field
 */
#ifndef USE_PTHREADS
#define KSTACK (64 * 1024)	/* ARM64 stack size */
static __inline Proc *getup(void) {
	Proc *p;
	__asm__(	"mov	%0, sp"
			: "=r" (p)
		);
	return *(Proc **)((uintptr)p & ~(KSTACK - 1));
}
#else
#define KSTACK (64 * 1024)
extern	Proc*	getup(void);
#endif

#define	up	(getup())

typedef sigjmp_buf osjmpbuf;
#define	ossetjmp(buf)	sigsetjmp(buf, 1)

/*
 * Android OS abstraction layer for TaijiOS
 * Ported from emu/Linux/os.c with Android-specific adaptations
 */

#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>
#include <android/log.h>

#include "dat.h"
#include "fns.h"
#include "error.h"

#define LOG_TAG "TaijiOS"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

enum
{
	DELETE	= 0x7f,
	CTRLC	= 'C'-'@',
	NSTACKSPERALLOC = 16,
	GLESSTACK= 256*1024
};

char *hosttype = "Android";

typedef struct {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int count;
} Sem;

extern int dflag;

int	gidnobody = -1;
int	uidnobody = -1;

/*
 * Android doesn't have the same signal handling as Linux
 * We use pthread primitives for synchronization
 */

static void
sysfault(char *what, void *addr)
{
	char buf[64];

	snprint(buf, sizeof(buf), "sys: %s%#p", what, addr);
	disfault(nil, buf);
}

static void
trapILL(int signo, siginfo_t *si, void *a)
{
	USED(signo);
	USED(a);
	sysfault("illegal instruction pc=", si->si_addr);
}

static int
isnilref(siginfo_t *si)
{
	return si != 0 && (si->si_addr == (void*)~(uintptr_t)0 || (uintptr_t)si->si_addr < 512);
}

static void
trapmemref(int signo, siginfo_t *si, void *a)
{
	USED(a);
	if(isnilref(si))
		disfault(nil, exNilref);
	else if(signo == SIGBUS)
		sysfault("bad address addr=", si->si_addr);
	else
		sysfault("segmentation violation addr=", si->si_addr);
}

static void
trapFPE(int signo, siginfo_t *si, void *a)
{
	char buf[64];

	USED(signo);
	USED(a);
	snprint(buf, sizeof(buf), "sys: fp: exception addr=%#p", si->si_addr);
	disfault(nil, buf);
}

/*
 * Android uses pthreads, so we use pthread condition for signaling
 */
static void
trapUSR1(int signo)
{
	int intwait;

	USED(signo);

	intwait = up->intwait;
	up->intwait = 0;

	if(up->type != Interp)
		return;

	if(intwait == 0)
		disfault(nil, Eintr);
}

void
oslongjmp(void *regs, osjmpbuf env, int val)
{
	USED(regs);
	siglongjmp(env, val);
}

void
cleanexit(int x)
{
	USED(x);

	if(up->intwait) {
		up->intwait = 0;
		return;
	}

	_exit(0);
}

void
osreboot(char *file, char **argv)
{
	execvp(file, argv);
	error("reboot failure");
}

void
libinit(char *imod)
{
	struct sigaction act;
	Proc *p;
	char sys[64];
	struct passwd *pw;

	setsid();

	gethostname(sys, sizeof(sys));
	kstrdup(&ossysname, sys);

	pw = getpwnam("nobody");
	if(pw != nil) {
		uidnobody = pw->pw_uid;
		gidnobody = pw->pw_gid;
	}

	/* Set up signal handlers for Android */
	memset(&act, 0, sizeof(act));
	act.sa_handler = SIG_IGN;
	sigaction(SIGCHLD, &act, nil);
	sigaction(SIGPIPE, &act, nil);

	if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, cleanexit);
	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, cleanexit);

	if(sflag == 0) {
		act.sa_flags = SA_SIGINFO;
		act.sa_sigaction = trapILL;
		sigaction(SIGILL, &act, nil);
		act.sa_sigaction = trapFPE;
		sigaction(SIGFPE, &act, nil);
		act.sa_sigaction = trapmemref;
		sigaction(SIGBUS, &act, nil);
		sigaction(SIGSEGV, &act, nil);
		act.sa_flags &= ~SA_SIGINFO;
	}

	p = newproc();
	kprocinit(p);

	pw = getpwuid(getuid());
	if(pw != nil)
		kstrdup(&eve, pw->pw_name);
	else
		print("cannot getpwuid\n");

	p->env->uid = getuid();
	p->env->gid = getgid();

	emuinit(imod);
}

/*
 * Android: use ADB or logcat for keyboard input
 */
int
readkbd(void)
{
	int n;
	char buf[1];

	n = read(0, buf, sizeof(buf));
	if(n < 0)
		print("keyboard close (n=%d, %s)\n", n, strerror(errno));
	if(n <= 0)
		pexit("keyboard thread", 0);

	switch(buf[0]) {
	case '\r':
		buf[0] = '\n';
		break;
	case DELETE:
		buf[0] = 'H' - '@';
		break;
	case CTRLC:
		cleanexit(0);
		break;
	}
	return buf[0];
}

/*
 * Return an arbitrary millisecond clock time
 */
long
osmillisec(void)
{
	static long sec0 = 0, usec0;
	struct timeval t;

	if(gettimeofday(&t, (struct timezone*)0) < 0)
		return 0;

	if(sec0 == 0) {
		sec0 = t.tv_sec;
		usec0 = t.tv_usec;
	}
	return (t.tv_sec - sec0)*1000 + (t.tv_usec - usec0 + 500)/1000;
}

/*
 * Return the time since the epoch in nanoseconds and microseconds
 */
vlong
osnsec(void)
{
	struct timeval t;

	gettimeofday(&t, nil);
	return (vlong)t.tv_sec*1000000000L + t.tv_usec*1000;
}

vlong
osusectime(void)
{
	struct timeval t;

	gettimeofday(&t, nil);
	return (vlong)t.tv_sec * 1000000 + t.tv_usec;
}

int
osmillisleep(ulong milsec)
{
	struct timespec time;

	time.tv_sec = milsec/1000;
	time.tv_nsec = (milsec%1000)*1000000;
	nanosleep(&time, NULL);
	return 0;
}

int
limbosleep(ulong milsec)
{
	return osmillisleep(milsec);
}

/*
 * Android uses pthread for yield
 */
void
osyield(void)
{
	sched_yield();
}

/*
 * Android block/unblock - simplified
 */
void
osblock(void)
{
	pthread_mutex_lock(&up->os->mutex);
	pthread_cond_wait(&up->os->cond, &up->os->mutex);
	pthread_mutex_unlock(&up->os->mutex);
}

void
osready(Proc *p)
{
	pthread_mutex_lock(&p->os->mutex);
	pthread_cond_signal(&p->os->cond);
	pthread_mutex_unlock(&p->os->mutex);
}

/*
 * Android: OS-specific enter/leave for critical sections
 */
void
osenter(void)
{
}

void
osleave(void)
{
}

void
oslopri(void)
{
	/* Android doesn't support changing thread priority easily */
}

/*
 * Host interrupt support via pthread_kill
 */
void
oshostintr(Proc *p)
{
	pthread_kill((pthread_t)p->os->tid, SIGUSR1);
}

/*
 * Pause - Android-specific
 */
void
ospause(void)
{
	pause();
}

/*
 * Semaphore operations using pthread
 */
void
ossemacquire(Sem *s)
{
	pthread_mutex_lock(&s->mutex);
	while(s->count <= 0)
		pthread_cond_wait(&s->cond, &s->mutex);
	s->count--;
	pthread_mutex_unlock(&s->mutex);
}

void
ossemrelease(Sem *s, int count)
{
	pthread_mutex_lock(&s->mutex);
	s->count += count;
	pthread_cond_broadcast(&s->cond);
	pthread_mutex_unlock(&s->mutex);
}

/*
 * Error handling
 */
void
oserror(void)
{
	oserrstr(up->env->errstr, ERRMAX);
	error(up->env->errstr);
}

void
oserrstr(char *buf, uint n)
{
	char *s;

	s = strerror(errno);
	strncpy(buf, s, n);
	buf[n-1] = 0;
}

/*
 * Command execution for Android
 */
void*
oscmd(char **argv, int nice, char *dir, int *pid)
{
	USED(dir);
	USED(nice);
	/* TODO: Implement for Android */
	return nil;
}

int
oscmdwait(void *cmd, char *buf, int n)
{
	USED(cmd);
	USED(buf);
	USED(n);
	return -1;
}

int
oscmdkill(void *cmd)
{
	USED(cmd);
	return -1;
}

void
oscmdfree(void *cmd)
{
	USED(cmd);
}

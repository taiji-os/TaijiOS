#ifndef LIB9_H
#define LIB9_H

/*
 * Basic type definitions for TaijiOS Android port
 * Based on Inferno's u.h and lib9.h for compatibility
 */

#define	USE_PTHREADS
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _XOPEN_SOURCE  500
#define _LARGEFILE_SOURCE	1
#define _LARGEFILE64_SOURCE	1
#define _FILE_OFFSET_BITS 64
#define _REENTRANT	1

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <setjmp.h>
#include <float.h>
#include <time.h>
#include <stddef.h>

#define	getwd	infgetwd

#ifndef nil
#define	nil		((void*)0)
#endif

typedef unsigned char	uchar;
typedef unsigned short	ushort;
typedef unsigned long	ulong;
typedef unsigned int	uint;
typedef signed char	schar;
typedef long long int	vlong;
typedef unsigned long long int	uvlong;
typedef unsigned int u32;
typedef uvlong u64int;

typedef unsigned int	mpdigit;	/* for /sys/include/mp.h */
typedef unsigned short u16int;
typedef unsigned char u8int;

typedef signed long long s64;
typedef unsigned long long u64int;
typedef signed long long s64int;
typedef	s64int		intptr;
typedef	u64int		uintptr;
typedef unsigned long long u64;
typedef unsigned int	u32int;
typedef signed int	s32int;
typedef unsigned int	u32;
typedef signed int s32;
typedef intptr		WORD;
typedef uintptr		UWORD;
typedef unsigned short	u16int;
typedef signed short	s16int;
typedef unsigned short	u16;
typedef signed short s16;

typedef unsigned char	u8int;
typedef unsigned char	u8;
typedef signed char	s8int;
typedef signed char s8;

typedef unsigned int Rune;
typedef struct Proc Proc;

#define	USED(x)		if(x){}else{}
#define	SET(x)

#undef nelem
#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))
#undef offsetof
#define	offsetof(s, m)	(uintptr)(&(((s*)0)->m))

/* Forward declarations */
typedef struct Fmt Fmt;
typedef struct Lock Lock;
typedef struct Qid Qid;
typedef struct Dir Dir;

struct Fmt{
	uchar	runes;			/* output buffer is runes or chars? */
	void	*start;			/* of buffer */
	void	*to;			/* current place in the buffer */
	void	*stop;			/* end of the buffer; overwritten if flush fails */
	int	(*flush)(Fmt *);	/* called when to == stop */
	void	*farg;			/* to make flush a closure */
	int	nfmt;			/* num chars formatted so far */
	va_list	args;			/* args passed to dofmt */
	int	r;			/* % format Rune */
	int	width;
	int	prec;
	u32	flags;
};

/* Lock structure */
struct Lock {
	int	val;
};

extern void lock(Lock*);
extern void unlock(Lock*);
extern int canlock(Lock*);

/* Fmt flag constants */
enum{
	FmtWidth	= 1,
	FmtLeft		= FmtWidth << 1,
	FmtPrec		= FmtLeft << 1,
	FmtSharp	= FmtPrec << 1,
	FmtSpace	= FmtSharp << 1,
	FmtSign		= FmtSpace << 1,
	FmtZero		= FmtSign << 1,
	FmtUnsigned	= FmtZero << 1,
	FmtShort	= FmtUnsigned << 1,
	FmtLong		= FmtShort << 1,
	FmtVLong	= FmtLong << 1,
	FmtComma	= FmtVLong << 1,
	FmtByte		= FmtComma << 1,
	FmtFlag		= FmtByte << 1
};

/* QLock structure */
typedef struct QLock QLock;
struct QLock {
	Lock	use;			/* to access Qlock structure */
	Proc	*head;			/* next process waiting for object */
	Proc	*tail;			/* last process waiting for object */
	int	locked;			/* flag */
};

extern void qlock(QLock*);
extern void qunlock(QLock*);
extern int canqlock(QLock*);

/* RWLock structure */
typedef struct RWLock RWLock;
struct RWLock {
	Lock	l;			/* Lock modify lock */
	QLock	x;			/* Mutual exclusion lock */
	QLock	k;			/* Lock for waiting writers */
	int	readers;		/* Count of readers in lock */
};

extern int canrlock(RWLock*);
extern int canwlock(RWLock*);
extern void rlock(RWLock*);
extern void runlock(RWLock*);
extern void wlock(RWLock*);
extern void wunlock(RWLock*);

/* Open mode constants */
#define	OREAD	0	/* open for read */
#define	OWRITE	1	/* write */
#define	ORDWR	2	/* read and write */
#define	OEXEC	3	/* execute, == read but check execute permission */
#define	OTRUNC	16	/* or'ed in (except for exec), truncate file first */
#define	OCEXEC	32	/* or'ed in, close on exec */
#define	ORCLOSE	64	/* or'ed in, remove on close */
#define	OEXCL	0x1000	/* or'ed in, exclusive use (create only) */

#define	AEXIST	0	/* accessible: exists */
#define	AEXEC	1	/* execute access */
#define	AWRITE	2	/* write access */
#define	AREAD	4	/* read access */

/* bits in Qid.type */
#define QTDIR		0x80		/* type bit for directories */
#define QTAPPEND	0x40		/* type bit for append only files */
#define QTEXCL		0x20		/* type bit for exclusive use files */
#define QTMOUNT		0x10		/* type bit for mounted channel */
#define QTAUTH		0x08		/* type bit for authentication file */
#define QTFILE		0x00		/* plain file */

/* bits in Dir.mode */
#define DMDIR		0x80000000	/* mode bit for directories */
#define DMAPPEND	0x40000000	/* mode bit for append only files */
#define DMEXCL		0x20000000	/* mode bit for exclusive use files */
#define DMMOUNT		0x10000000	/* mode bit for mounted channel */
#define DMAUTH		0x08000000	/* mode bit for authentication file */
#define DMREAD		0x4		/* mode bit for read permission */
#define DMWRITE		0x2		/* mode bit for write permission */
#define DMEXEC		0x1		/* mode bit for execute permission */

/* Qid structure */
struct Qid {
	u64int	path;
	u32	vers;
	uchar	type;
};

/* Dir structure */
struct Dir {
	/* system-modified data */
	ushort	type;	/* server type */
	uint	dev;	/* server subtype */
	/* file data */
	Qid	qid;	/* unique id from server */
	u32	mode;	/* permissions */
	u32	atime;	/* last read time */
	u32	mtime;	/* last write time */
	s64int	length;	/* file length */
	char	*name;	/* last element of path */
	char	*uid;	/* owner name */
	char	*gid;	/* group name */
	char	*muid;	/* last modifier name */
};

/* FPtrickle for mptod.c */
#ifndef FPdbleword_defined
#define FPdbleword_defined
typedef union FPdbleword FPdbleword;
union FPdbleword {
	double	x;
	struct {
		uint lo;
		uint hi;
	};
};
#endif

/* Function declarations */
extern	char*	strecpy(char*, char*, char*);
extern	char*	strdup(const char*);
extern	int	cistrncmp(char*, char*, int);
extern	int	cistrcmp(char*, char*);
extern	char*	cistrstr(char*, char*);
extern	int	tokenize(char*, char**, int);

extern	int	print(char*, ...);
extern	char*	seprint(char*, char*, char*, ...);
extern	char*	vseprint(char*, char*, char*, va_list);
extern	int	snprint(char*, int, char*, ...);
extern	int	vsnprint(char*, int, char*, va_list);
extern	char*	smprint(char*, ...);
extern	char*	vsmprint(char*, va_list);
extern	int	sprint(char*, char*, ...);
extern	int	fprint(int, char*, ...);
extern	int	vfprint(int, char*, va_list);

extern	int	fmtfdinit(Fmt*, int, char*, int);
extern	int	fmtfdflush(Fmt*);
extern	int	fmtstrinit(Fmt*);
extern	char*	fmtstrflush(Fmt*);

extern	int	fmtinstall(int, int (*)(Fmt*));
extern	int	dofmt(Fmt*, char*);
extern	int	dorfmt(Fmt*, Rune*);
extern	int	fmtprint(Fmt*, char*, ...);
extern	int	fmtvprint(Fmt*, char*, va_list);
extern	int	fmtrune(Fmt*, int);
extern	int	fmtstrcpy(Fmt*, char*);

extern int errfmt(Fmt *f);
extern void werrstr(char*, ...);
extern int errstr(char*, uint);
extern void rerrstr(char*, uint);

extern void* mallocz(size_t, int);
extern size_t msize(void*);
extern void perror(const char*);
extern long readn(int, void*, long);
extern vlong seek(int, vlong, int);
extern int segflush(void*, ulong);

extern int create(char*, int, int);
extern void exits(char*);
extern void _exits(char*);

extern Dir* dirstat(char*);
extern Dir* dirfstat(int);
extern int dirwstat(char*, Dir*);
extern int dirfwstat(int, Dir*);
extern s32 dirread(int, Dir**);
extern void nulldir(Dir*);
extern s32 dirreadall(int, Dir**);

extern char *argv0;

/* UTF constants */
enum
{
	UTFmax		= 4,		/* maximum bytes per rune */
	Runesync	= 0x80,		/* cannot represent part of a UTF sequence (<) */
	Runeself	= 0x80,		/* rune and UTF sequences are the same (<) */
	Runeerror	= 0xFFFD,	/* decoding error in UTF */
	Runemax		= 0x10FFFF,	/* 21-bit rune */
	Runemask	= 0x1FFFFF,	/* bits used by runes (see grep) */
};

/* Mount flags */
#define	MORDER	0x0003	/* mask for bits defining order of mounting */
#define	MREPL	0x0000	/* mount replaces object */
#define	MBEFORE	0x0001	/* mount goes before others in union directory */
#define	MAFTER	0x0002	/* mount goes after others in union directory */
#define	MCREATE	0x0004	/* permit creation in mounted directory */
#define	MCACHE	0x0010	/* cache some data */
#define	MMASK	0x0017	/* all bits on */

/* Rune functions */
extern	int	runetochar(char*, Rune*);
extern	int	chartorune(Rune*, char*);
extern	int	runelen(long);
extern	int	runenlen(Rune*, int);
extern	int	fullrune(char*, int);
extern	int	utflen(char*);
extern	int	utfnlen(char*, long);
extern	char*	utfrune(char*, long);
extern	char*	utfrrune(char*, long);
extern	char*	utfutf(char*, char*);
extern	char*	utfecpy(char*, char*, char*);

extern	Rune*	runestrcat(Rune*, Rune*);
extern	Rune*	runestrchr(Rune*, Rune);
extern	int	runestrcmp(Rune*, Rune*);
extern	Rune*	runestrcpy(Rune*, Rune*);
extern	Rune*	runestrncpy(Rune*, Rune*, int);
extern	Rune*	runestrecpy(Rune*, Rune*, Rune*);
extern	Rune*	runestrdup(Rune*);
extern	Rune*	runestrcat(Rune*, Rune*);
extern	int	runestrncmp(Rune*, Rune*, int);
extern	Rune*	runestrrchr(Rune*, Rune);
extern	long	runestrlen(Rune*);
extern	Rune*	runestrstr(Rune*, Rune*);

/* assert macro */
#undef assert
#define	assert(x)	if(x){}else

#define	ARGBEGIN	for((argv0||(argv0=*argv)),argv++,argc--;\
			    argv[0] && argv[0][0]=='-' && argv[0][1];\
			    argc--, argv++) {\
				char *_args, *_argt;\
				Rune _argc;\
				_args = &argv[0][1];\
				if(_args[0]=='-' && _args[1]==0){\
					argc--; argv++; break;\
				}\
				_argc = 0;\
				while(*_args && (_args += chartorune(&_argc, _args)))\
				switch(_argc)
#define	ARGEND		SET(_argt);USED(_argt);USED(_argc); USED(_args);}USED(argv); USED(argc);
#define	ARGF()		(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): 0))
#define	EARGF(x)	(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): ((x), abort(), (char*)0)))
#define	ARGC()		_argc

#define	STATMAX	65535U
#define	DIRMAX	(sizeof(Dir)+STATMAX)
#define	ERRMAX	128

#endif /* LIB9_H */

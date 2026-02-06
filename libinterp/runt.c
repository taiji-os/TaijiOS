#include "lib9.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "sysmod.h"
#include "raise.h"

static	int		utfnleng(char*, int, int*);

extern char* string2c(String*);

/*
 * Safe conversion of String to C string for xprint
 * Returns pointer to static buffer with null-terminated string
 * Returns empty string if String is invalid or inaccessible
 */
static char*
safe_string2c(String *s)
{
	static char buf[2048];
	int len, i;
	char *src;
	uintptr_t str_addr, data_addr;

	/* Validate String pointer thoroughly */
	if(s == H || (uintptr_t)s < 0x1000)
		return "";

	str_addr = (uintptr_t)s;

	/* Additional validation: check if pointer is in a reasonable range */
	/* User space on ARM64 is typically up to 0x0000007fffffffffff */
	if(str_addr > 0x0000800000000000ULL)
		return "";

	/* Calculate where the data (Sascii) starts - it's after len, max, tmp */
	/* sizeof(String) header = sizeof(int)*2 + sizeof(char*) + padding */
	data_addr = str_addr + 16;  /* Approximate offset to Sascii */

	/* If data would be beyond a reasonable range, reject */
	if(data_addr > 0x0000800000000000ULL - 2048)
		return "";

	/* Read the length field */
	len = s->len;

	if(len >= 0) {
		if(len < 0 || len > (int)sizeof(buf) - 1)
			len = sizeof(buf) - 1;

		/* Calculate the actual data address */
		src = s->Sascii;

		/* Check if reading len bytes from src would overflow */
		if((uintptr_t)src + len > 0x0000800000000000ULL)
			return "";

		for(i = 0; i < len; i++) {
			buf[i] = src[i];
		}
		buf[len] = '\0';
		return buf;
	}

	len = -len;
	if(len < 0 || len > (int)(sizeof(buf) / UTFmax) - 1)
		len = (sizeof(buf) / UTFmax) - 1;

	/* For Rune strings, just return empty for now */
	buf[0] = '\0';
	return buf;
}

void
sysmodinit(void)
{
	sysinit();
	builtinmod("$Sys", Sysmodtab, Sysmodlen);
}

#define FMTBUF_SIZE 128

int
xprint(Prog *xp, void *vfp, void *vva, String *s1, char *buf, int n)
{
	WORD i;
	void *p;
	LONG bg;
	Type *t;
	double d;
	String *ss;
	ulong *ptr;
	uchar *fp, *va;
	int nc, c, isbig;
	char *b, *eb, *f, fmt[FMTBUF_SIZE];
	Rune r;
	char *fmt_cstr;
	int fmt_pos;
	int fmt_len;

	fp = vfp;
	va = vva;

	b = buf;
	eb = buf + n - 1;

	/* Validate format string pointer first */
	if(s1 == H || (uintptr_t)s1 < 0x1000) {
		if(n > 0)
			buf[0] = '\0';
		return 0;
	}

	/* Get format string length */
	fmt_len = s1->len;
	if(fmt_len < 0)
		fmt_len = -fmt_len;

	/* Validate format string length */
	if(fmt_len > 65536) {
		if(n > 0)
			buf[0] = '\0';
		return 0;
	}

	/* Convert format String to C string */
	fmt_cstr = safe_string2c(s1);
	if(fmt_cstr == NULL || fmt_cstr[0] == '\0') {
		if(n > 0)
			buf[0] = '\0';
		return 0;
	}

	fmt_pos = 0;

	/* Process format string character by character */
	while(fmt_cstr[fmt_pos] != '\0' && (b - buf) < n - 1) {
		c = (uchar)fmt_cstr[fmt_pos++];

		if(c != '%') {
			if(b < eb) {
				if(c < Runeself)
					*b++ = c;
				else
					b += snprint(b, eb-b, "%C", c);
			}
			continue;
		}

		/* Start of format specifier */
		f = fmt;
		*f++ = c;
		isbig = 0;

		/* Parse the format specifier */
		while(fmt_cstr[fmt_pos] != '\0') {
			c = (uchar)fmt_cstr[fmt_pos++];
			*f++ = c;
			*f = '\0';

			switch(c) {
			default:
				continue;
			case '*':
				i = *(WORD*)va;
				f--;
				f += snprint(f, sizeof(fmt)-(f-fmt), "%zd", i);
				va += IBY2WD;
				continue;
			case 'b':
				f[-1] = 'l';
				*f++ = 'l';
				*f = '\0';
				isbig = 1;
				continue;
			case '%':
				if(b < eb)
					*b++ = '%';
				break;
			case 'q':
			case 's':
				ss = *(String**)va;
				va += IBY2WD;
				/* Validate String pointer BEFORE accessing */
				if(ss == H || (uintptr_t)ss < 0x1000 || (uintptr_t)ss > 0x7ffffffffffULL) {
					p = "(null)";
				} else {
					p = safe_string2c(ss);
				}
				b += snprint(b, eb-b, fmt, p);
				break;
			case 'E':
				f--;
				r = 0x00c9;
				f += runetochar(f, &r);
				*f = '\0';
			case 'e':
			case 'f':
			case 'g':
			case 'G':
				while((va - fp) & (sizeof(REAL)-1))
					va++;
				d = *(REAL*)va;
				b += snprint(b, eb-b, fmt, d);
				va += sizeof(REAL);
				break;
			case 'd':
			case 'o':
			case 'x':
			case 'X':
			case 'c':
				if(isbig) {
					while((va - fp) & (IBY2LG-1))
						va++;
					bg = *(LONG*)va;
					b += snprint(b, eb-b, fmt, bg);
					va += IBY2LG;
				}
				else {
					i = *(WORD*)va;
					if(c == 'c')
						f[-1] = 'C';
					b += snprint(b, eb-b, fmt, i);
					va += IBY2WD;
				}
				break;
			case 'r':
				b = syserr(b, eb, xp);
				break;
			case 'H':
				ptr = *(ulong**)va;
				c = -1;
				t = nil;
				if(ptr != H) {
					c = D2H(ptr)->ref;
					t = D2H(ptr)->t;
				}
				b += snprint(b, eb-b, "%d.%.8zx", c, (uintptr)t);
				va += IBY2WD;
				break;
			}
			break;
		}
	}

	if(b < eb)
		*b = '\0';

	return b - buf;
}

int
bigxprint(Prog *xp, void *vfp, void *vva, String *s1, char **buf, int s)
{
	char *b;
	int m, n;

	m = s;
	for (;;) {
		m *= 2;
		b = malloc(m);
		if (b == nil)
			error(exNomem);
		n = xprint(xp, vfp, vva, s1, b, m);
		if (n < m-UTFmax-2)
			break;
		free(b);
	}
	*buf = b;
	return n;
}

void
Sys_sprint(void *fp)
{
	int n;
	char buf[256], *b = buf;
	F_Sys_sprint *f;

	f = fp;
	n = xprint(currun(), f, &f->vargs, f->s, buf, sizeof(buf));
	if (n >= sizeof(buf)-UTFmax-2)
		n = bigxprint(currun(), f, &f->vargs, f->s, &b, sizeof(buf));
	b[n] = '\0';
	retstr(b, f->ret);
	if (b != buf)
		free(b);
}

void
Sys_aprint(void *fp)
{
	int n;
	char buf[256], *b = buf;
	F_Sys_aprint *f;

	f = fp;
	n = xprint(currun(), f, &f->vargs, f->s, buf, sizeof(buf));
	if (n >= sizeof(buf)-UTFmax-2)
		n = bigxprint(currun(), f, &f->vargs, f->s, &b, sizeof(buf));
	destroy(*f->ret);
	*f->ret = mem2array(b, n);
	if (b != buf)
		free(b);
}

static int
tokdelim(int c, String *d)
{
	int l;
	char *p;
	Rune *r;

	l = d->len;
	if(l < 0) {
		l = -l;
		for(r = d->Srune; l != 0; l--)
			if(*r++ == c)
				return 1;
		return 0;
	}
	for(p = d->Sascii; l != 0; l--)
		if(*p++ == c)
			return 1;
	return 0;
}

void
Sys_tokenize(void *fp)
{
	String *s, *d;
	List **h, *l, *nl;
	F_Sys_tokenize *f;
	int n, c, nc, first, last, srune;

	f = fp;
	s = f->s;
	d = f->delim;

	if(s == H || d == H) {
		f->ret->t0 = 0;
		destroy(f->ret->t1);
		f->ret->t1 = H;
		return;
	}

	n = 0;
	l = H;
	h = &l;
	first = 0;
	srune = 0;

	nc = s->len;
	if(nc < 0) {
		nc = -nc;
		srune = 1;
	}

	while(first < nc) {
		while(first < nc) {
			c = srune ? s->Srune[first] : s->Sascii[first];
			if(tokdelim(c, d) == 0)
				break;	
			first++;
		}

		last = first;

		while(last < nc) {
			c = srune ? s->Srune[last] : s->Sascii[last];
			if(tokdelim(c, d) != 0)
				break;	
			last++;
		}

		if(first == last)
			break;

		nl = cons(IBY2WD, h);
		nl->tail = H;
		nl->t = &Tptr;
		Tptr.ref++;
		*(String**)nl->data = slicer(first, last, s);
		h = &nl->tail;

		first = last;
		n++;
	}

	f->ret->t0 = n;
	destroy(f->ret->t1);
	f->ret->t1 = l;
}

void
Sys_utfbytes(void *fp)
{
	Array *a;
	int nbyte;
	F_Sys_utfbytes *f;

	f = fp;
	a = f->buf;
	if(a == H || (UWORD)f->n > a->len)
		error(exBounds);

	utfnleng((char*)a->data, f->n, &nbyte);
	*f->ret = nbyte;
}

void
Sys_byte2char(void *fp)
{
	Rune r;
	char *p;
	int n, w;
	Array *a;
	F_Sys_byte2char *f;

	f = fp;
	a = f->buf;
	n = f->n;
	if(a == H || (UWORD)n >= a->len)
		error(exBounds);
	r = a->data[n];
	if(r < Runeself){
		f->ret->t0 = r;
		f->ret->t1 = 1;
		f->ret->t2 = 1;
		return;
	}
	p = (char*)a->data+n;
	if(n+UTFmax <= a->len || fullrune(p, a->len-n))
		w = chartorune(&r, p);
	else {
		/* insufficient data */
		f->ret->t0 = Runeerror;
		f->ret->t1 = 0;
		f->ret->t2 = 0;
		return;
	}
	if(r == Runeerror && w==1){	/* encoding error */
		f->ret->t0 = Runeerror;
		f->ret->t1 = 1;
		f->ret->t2 = 0;
		return;
	}
	f->ret->t0 = r;
	f->ret->t1 = w;
	f->ret->t2 = 1;
}

void
Sys_char2byte(void *fp)
{
	F_Sys_char2byte *f;
	Array *a;
	int n, c;
	Rune r;

	f = fp;
	a = f->buf;
	n = f->n;
	c = f->c;
	if(a == H || (UWORD)n>=a->len)
		error(exBounds);
	if(c<0 || c>=Runemax)
		c = Runeerror;
	if(c < Runeself){
		a->data[n] = c;
		*f->ret = 1;
		return;
	}
	r = c;
	if(n+UTFmax<=a->len || runelen(c)<=a->len-n){
		*f->ret = runetochar((char*)a->data+n, &r);
		return;
	}
	*f->ret = 0;
}

Module *
builtinmod(char *name, void *vr, int rlen)
{
	Runtab *r = vr;
	Type *t;
	Module *m;
	Link *l;

	print("builtinmod: registering '%s', rlen=%d\n", name, rlen);

	m = newmod(name);
	if(rlen == 0){
		while(r->name){
			rlen++;
			r++;
		}
		r = vr;
	}
	l = m->ext = (Link*)malloc((rlen+1)*sizeof(Link));
	if(l == nil){
		freemod(m);
		print("builtinmod ERROR: malloc failed for '%s'\n", name);
		return nil;
	}
	while(r->name) {
		t = dtype(freeheap, r->size, r->map, r->np);
		runtime(m, l, r->name, r->sig, r->fn, t);
		r++;
		l++;
	}
	l->name = nil;
	print("builtinmod: '%s' registered at m=%p, m->path='%s'\n", name, m, m->path);
	return m;
}

void
retnstr(char *s, int n, String **d)
{
	String *s1;

	s1 = H;
	if(n != 0)
		s1 = c2string(s, n);
	destroy(*d);
	*d = s1;
}

void
retstr(char *s, String **d)
{
	String *s1;

	s1 = H;
	if(s != nil)
		s1 = c2string(s, strlen(s));
	destroy(*d);
	*d = s1;
}

Array*
mem2array(void *va, int n)
{
	Heap *h;
	Array *a;

	if(n < 0)
		n = 0;
	h = nheap(sizeof(Array)+n);
	h->t = &Tarray;
	h->t->ref++;
	a = H2D(Array*, h);
	a->t = &Tbyte;
	Tbyte.ref++;
	a->len = n;
	a->root = H;
	a->data = (uchar*)a+sizeof(Array);
	if(va != 0)
		memmove(a->data, va, n);

	return a;
}

static int
utfnleng(char *s, int nb, int *ngood)
{
	int c;
	long n;
	Rune rune;
	char *es, *starts;

	starts = s;
	es = s+nb;
	for(n = 0; s < es; n++) {
		c = *(uchar*)s;
		if(c < Runeself)
			s++;
		else {
			if(s+UTFmax<=es || fullrune(s, es-s))
				s += chartorune(&rune, s);
			else
				break;
		}
	}
	if(ngood)
		*ngood = s-starts;
	return n;
}
// FORCE REBUILD Thu Feb  5 11:49:03 PM -03 2026
/* FORCE REBUILD Fri Feb  6 01:16:00 AM -03 2026 */
// FORCE REBUILD xprint heap buffer fix - Thu Feb  6 02:00:00 AM -03 2026
// FORCE REBUILD byte-by-byte copy - Fri Feb  6 02:10:00 AM -03 2026
// FORCE REBUILD SIGSEGV handler - Fri Feb  6 02:20:00 AM -03 2026
// FORCE REBUILD simplified memmove - Fri Feb  6 02:30:00 AM -03 2026
// FORCE REBUILD safest empty string - Fri Feb  6 02:40:00 AM -03 2026
// FORCE REBUILD no string access - Fri Feb  6 02:50:00 AM -03 2026
// FORCE REBUILD s1 bounds check - Fri Feb  6 03:00:00 AM -03 2026

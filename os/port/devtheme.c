/*
 *	devtheme.c - /dev/theme device driver for TaijiOS theming
 *
 *	Provides a Plan 9-style interface to global UI theme colors.
 *	Theme colors are exposed as numbered files (0-16) and as named
 *	control files for theme management.
 *
 *	Usage:
 *		cat /dev/theme/ctl       - read current theme info
 *		echo 'dark' > /dev/theme/theme  - load a theme
 *		cat /dev/theme/1         - read background color
 *		echo '#FF0000FF' > /dev/theme/1  - set background color
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#define NTHEMECOLORS  17

/* QID path values for theme files */
enum {
	Qctl,
	Qtheme,
	Qlist,
	Qreload,
	Qevent,
	Qcolor0,   /* TkCforegnd */
	Qcolor1,   /* TkCbackgnd */
	Qcolor2,   /* TkCbackgndlght */
	Qcolor3,   /* TkCbackgnddark */
	Qcolor4,   /* TkCselect */
	Qcolor5,   /* TkCselectbgnd */
	Qcolor6,   /* TkCselectbgndlght */
	Qcolor7,   /* TkCselectbgnddark */
	Qcolor8,   /* TkCselectfgnd */
	Qcolor9,   /* TkCactivebgnd */
	Qcolor10,  /* TkCactivebgndlght */
	Qcolor11,  /* TkCactivebgnddark */
	Qcolor12,  /* TkCactivefgnd */
	Qcolor13,  /* TkCdisablefgnd */
	Qcolor14,  /* TkChighlightfgnd */
	Qcolor15,  /* TkCfill */
	Qcolor16,  /* TkCtransparent */
};

/* Theme color state */
typedef struct ThemeColor {
	char *name;
	ulong value;
	int vers;
} ThemeColor;

typedef struct ThemeState {
	QLock q;
	ThemeColor colors[NTHEMECOLORS];
	int nlisteners;
	Rendez eventq;
	char current_theme[64];
	uvlong version;
} ThemeState;

static ThemeState themestate;

/* Dirtab for static files */
static Dirtab themedirtab[] = {
	"ctl",        {Qctl}, 0, 0666,
	"theme",      {Qtheme}, 0, 0666,
	"list",       {Qlist}, 0, 0444,
	"reload",     {Qreload}, 0, 0222,
	"event",      {Qevent}, 0, 0444,
};

/* Color names matching TkC indices */
static char* colornames[NTHEMECOLORS] = {
	"foreground",           /* TkCforegnd */
	"background",           /* TkCbackgnd */
	"background_light",     /* TkCbackgndlght */
	"background_dark",      /* TkCbackgnddark */
	"select",               /* TkCselect */
	"select_background",    /* TkCselectbgnd */
	"select_background_light", /* TkCselectbgndlght */
	"select_background_dark",  /* TkCselectbgnddark */
	"select_foreground",    /* TkCselectfgnd */
	"active_background",    /* TkCactivebgnd */
	"active_background_light", /* TkCactivebgndlght */
	"active_background_dark",  /* TkCactivebgnddark */
	"active_foreground",    /* TkCactivefgnd */
	"disabled_foreground",  /* TkCdisablefgnd */
	"highlight_foreground", /* TkChighlightfgnd */
	"fill",                 /* TkCfill */
	"transparent",          /* TkCtransparent */
};

/* Default colors from libtk/colrs.c */
static ulong defaultcolors[NTHEMECOLORS] = {
	0x000000FF, /* foreground */
	0xDDDDDDFF, /* background */
	0xEEEEEEFF, /* background_light */
	0xC8C8C8FF, /* background_dark */
	0xB03060FF, /* select */
	0x404040FF, /* select_background */
	0x505050FF, /* select_background_light */
	0x303030FF, /* select_background_dark */
	0xFFFFFFFF, /* select_foreground */
	0xEDEDEDFF, /* active_background */
	0xFEFEFEFF, /* active_background_light */
	0xD8D8D8FF, /* active_background_dark */
	0x000000FF, /* active_foreground */
	0x888888FF, /* disabled_foreground */
	0x000000FF, /* highlight_foreground */
	0xDDDDDDFF, /* fill */
	0x00000000, /* transparent */
};

static int load_theme_by_name(char *name);
static void notify_listeners(void);
static int scandir_themes(char *buf, int n);
static void save_theme_name(char *name);
static void load_saved_theme(void);

static void
themeinit(void)
{
	int i;

	memset(&themestate, 0, sizeof(ThemeState));

	for(i = 0; i < NTHEMECOLORS; i++) {
		themestate.colors[i].name = colornames[i];
		themestate.colors[i].value = defaultcolors[i];
		themestate.colors[i].vers = 0;
	}

	strcpy(themestate.current_theme, "default");
	themestate.version = 0;

	/* Try to load saved theme */
	load_saved_theme();
}

/* Scan /lib/theme directory for .theme files using native Inferno APIs */
static int
scandir_themes(char *buf, int n)
{
	Chan *c;
	char *p = buf;
	int left = n;
	int i, nentries;
	Dir *entries;
	char *dot;

	c = namec("/lib/theme", Aopen, OREAD, 0);
	if(c == nil)
		return snprint(buf, n, "default\ndark\n");  /* fallback */

	/* First call to get count */
	nentries = dirreadall(c, &entries);
	cclose(c);

	if(nentries <= 0) {
		return snprint(buf, n, "default\ndark\n");  /* fallback */
	}

	for(i = 0; i < nentries; i++) {
		char *name = entries[i].name;
		int len = strlen(name);

		/* Check for .theme extension */
		if(len >= 6) {
			dot = name + len - 6;
			if(strcmp(dot, ".theme") == 0) {
				int namelen = len - 6;
				if(namelen + 2 > left)
					break;
				memmove(p, name, namelen);
				p[namelen] = '\n';
				p += namelen + 1;
				left -= namelen + 1;
			}
		}
	}

	free(entries);
	return p - buf;
}

static Chan*
themeattach(char *spec)
{
	return devattach('w', spec);
}

static Walkqid*
themewalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, themedirtab, nelem(themedirtab), devgen);
}

static int
themestat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, themedirtab, nelem(themedirtab), devgen);
}

static Chan*
themeopen(Chan *c, int omode)
{
	return devopen(c, omode, themedirtab, nelem(themedirtab), devgen);
}

static void
themeclose(Chan *c)
{
	USED(c);
}

static s32
themeread(Chan *c, void *buf, s32 n, s64 off)
{
	char tmp[128];
	ulong path = c->qid.path;

	switch(path) {
	case Qctl:
		snprint(tmp, sizeof(tmp),
			"theme %s\nversion %lld\n",
			themestate.current_theme,
			themestate.version);
		return readstr(off, buf, n, tmp);

	case Qtheme:
		return readstr(off, buf, n, themestate.current_theme);

	case Qlist:
	{
		char listbuf[512];
		int len;
		len = scandir_themes(listbuf, sizeof(listbuf));
		return readstr(off, buf, n, listbuf);
	}

	case Qevent:
		/* Block until theme change - simple poll for now */
		return 0;

	default:
		/* Read color value */
		if(path >= Qcolor0 && path <= Qcolor0 + NTHEMECOLORS - 1) {
			int idx = path - Qcolor0;
			snprint(tmp, sizeof(tmp), "#%08ulX\n", themestate.colors[idx].value);
			return readstr(off, buf, n, tmp);
		}
	}

	return 0;
}

static s32
themewrite(Chan *c, void *buf, s32 n, s64 off)
{
	char str[128];
	ulong path = c->qid.path;
	ulong color;
	char *p;

	USED(off);

	if(n >= sizeof(str))
		n = sizeof(str) - 1;
	memmove(str, buf, n);
	str[n] = 0;

	switch(path) {
	case Qtheme:
		/* Load theme by name */
		p = str;
		while(*p == ' ' || *p == '\t' || *p == '\n') p++;
		if(strlen(p) > 0 && p[strlen(p)-1] == '\n')
			p[strlen(p)-1] = 0;
		if(load_theme_by_name(p) < 0)
			return -1;
		return n;

	case Qreload:
		/* Reload current theme - just notify listeners */
		notify_listeners();
		return n;

	default:
		/* Write color value */
		if(path >= Qcolor0 && path <= Qcolor0 + NTHEMECOLORS - 1) {
			int idx = path - Qcolor0;

			p = str;
			while(*p == ' ' || *p == '\t') p++;
			if(*p == '#') {
				color = strtoul(p+1, nil, 16);

				qlock(&themestate.q);
				themestate.colors[idx].value = color;
				themestate.colors[idx].vers++;
				themestate.version++;
				qunlock(&themestate.q);

				notify_listeners();
				return n;
			}
		}
	}

	return -1;
}

static int
load_theme_by_name(char *name)
{
	Chan *c;
	char path[128];
	char *line, *p, *key, *val;
	int i, n, fd;
	ulong color;
	char buf[1024];

	if(name == nil || strlen(name) == 0)
		return -1;

	/* Try /usr/theme first, then /lib/theme */
	snprint(path, sizeof(path), "/usr/theme/%s.theme", name);
	c = namec(path, Aopen, OREAD, 0);
	if(c == nil) {
		snprint(path, sizeof(path), "/lib/theme/%s.theme", name);
		c = namec(path, Aopen, OREAD, 0);
	}
	if(c == nil)
		return -1;

	qlock(&themestate.q);

	/* Read and parse theme file */
	fd = c->fid;
	while((n = sysread(fd, buf, sizeof(buf)-1)) > 0) {
		buf[n] = 0;
		line = buf;

		while((p = strchr(line, '\n')) != nil) {
			*p++ = 0;

			/* Skip comments and empty lines */
			while(*line == ' ' || *line == '\t') line++;
			if(*line == '#' || *line == 0) {
				line = p;
				continue;
			}

			/* Parse key = value */
			key = line;
			val = strchr(line, '=');
			if(val != nil) {
				*val++ = 0;
				while(*val == ' ' || *val == '\t') val++;

				/* Parse color */
				if(*val == '#') {
					color = strtoul(val+1, nil, 16);
					for(i = 0; i < NTHEMECOLORS; i++) {
						if(strcmp(themestate.colors[i].name, key) == 0) {
							themestate.colors[i].value = color;
							themestate.colors[i].vers++;
							break;
						}
					}
				}
			}
			line = p;
		}
	}

	cclose(c);

	strncpy(themestate.current_theme, name, sizeof(themestate.current_theme)-1);
	themestate.version++;

	qunlock(&themestate.q);

	/* Save theme name to persistent storage */
	save_theme_name(name);

	notify_listeners();
	return 0;
}

/* Save theme name to persistent storage */
static void
save_theme_name(char *name)
{
	Chan *c;

	c = namec("/nvfs/theme", Acreate, OWRITE, 0664);
	if(c != nil) {
		syswrite(c->fid, name, strlen(name));
		cclose(c);
	}
}

/* Load saved theme name from persistent storage */
static void
load_saved_theme(void)
{
	Chan *c;
	char name[64];
	s32 n;

	c = namec("/nvfs/theme", Aopen, OREAD, 0);
	if(c == nil)
		return;  /* No saved theme, use default */

	n = sysread(c->fid, name, sizeof(name)-1);
	cclose(c);

	if(n <= 0)
		return;

	/* Strip newline if present */
	if(name[n-1] == '\n')
		n--;
	name[n] = 0;

	/* Try to load the theme - if it fails, we keep the default */
	/* Note: we can't call load_theme_by_name here because the system
	 * isn't fully initialized yet. Just set the current_theme name.
	 */
	strncpy(themestate.current_theme, name, sizeof(themestate.current_theme)-1);
	themestate.current_theme[sizeof(themestate.current_theme)-1] = 0;
}

static void
notify_listeners(void)
{
	wakeup(&themestate.eventq);
}

Dev themedevtab = {
	'w',
	"theme",

	devreset,
	themeinit,
	devshutdown,
	themeattach,
	themewalk,
	themestat,
	themeopen,
	devcreate,
	themeclose,
	themeread,
	devbread,
	themewrite,
	devbwrite,
	devremove,
	devwstat
};

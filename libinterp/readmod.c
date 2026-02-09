#include "lib9.h"
#include "isa.h"
#include "interp.h"
#include "kernel.h"
#include "dynld.h"

/* Android logging */
#ifdef __ANDROID__
#include <android/log.h>
#define READMOD_LOG(...) __android_log_print(ANDROID_LOG_INFO, "TaijiOS-Readmod", __VA_ARGS__)
#else
#define READMOD_LOG(...)
#endif

static int debug = 0;

/*
 * android_module_redirect - Redirect module paths for Android-specific variants
 * Returns a new path that should be used instead of the original path.
 * Caller must free the returned path if it differs from the input.
 *
 * On Android, some modules have Android-specific implementations:
 * - wmlib.dis -> wmlib-android.dis (uses /dev/wmctx-* instead of /mnt/wm)
 */
static char*
android_module_redirect(const char *path)
{
	READMOD_LOG("android_module_redirect: called with path='%s'", path ? path : "(nil)");
#ifdef __ANDROID__
	/* Redirect wmlib.dis to wmlib-android.dis */
	if(path != nil && strcmp(path, "/dis/lib/wmlib.dis") == 0) {
		READMOD_LOG("android_module_redirect: Redirecting %s to /dis/lib/wmlib-android.dis", path);
		return strdup("/dis/lib/wmlib-android.dis");
	}
#endif
	READMOD_LOG("android_module_redirect: no redirect, returning nil");
	return nil;
}

Module*
readmod(char *path, Module *m, int sync)
{
	Dir *d;
	int fd, n, dynld;
	uchar *code;
	Module *ans;
	u32 length;
	char *redirected_path;
	int path_needs_free;

	print("readmod: path='%s', m=%p, sync=%d\n", path, m, sync);
	READMOD_LOG("readmod: path='%s', m=%p, sync=%d", path, m, sync);
	READMOD_LOG("readmod: ABOUT TO CALL android_module_redirect");

	/* Check for Android-specific module redirects */
	redirected_path = android_module_redirect(path);
	READMOD_LOG("readmod: AFTER android_module_redirect, redirected_path=%p", redirected_path);
	if(redirected_path != nil) {
		path_needs_free = 1;
		READMOD_LOG("readmod: Using redirected path: %s -> %s", path, redirected_path);
		path = redirected_path;
	} else {
		path_needs_free = 0;
	}

	if(path[0] == '$') {
		print("readmod: built-in module path, m=%p\n", m);
		if(m == nil) {
			kwerrstr("module not built-in");
			print("readmod ERROR: built-in module '%s' not found (m==nil)\n", path);
		}
		return m;
	}

	ans = nil;
	code = nil;
	length = 0;
	dynld = 0;

	if(sync)
		release();

	d = nil;
	fd = kopen(path, OREAD);
	if(fd < 0){
		READMOD_LOG("readmod: kopen FAILED for %s", path);
		DBG("readmod path %s, fd < 0\n", path);
		goto done;
	}
	READMOD_LOG("readmod: kopen succeeded fd=%d for %s", fd, path);

	if((d = kdirfstat(fd)) == nil){
		DBG("readmod (d = kdirfstat(fd)) == nil for path %s\n", path);
		goto done;
	}

	if(m != nil) {
		if(d->dev == m->dev && d->type == m->dtype &&
		   d->mtime == m->mtime &&
		   d->qid.type == m->qid.type && d->qid.path == m->qid.path && d->qid.vers == m->qid.vers) {
			DBG("readmod check failed for path %s\n", path);
			ans = m;
			goto done;
		}
	}

	if(d->length < 0 || d->length >= 8*1024*1024){
		kwerrstr("implausible length");
		goto done;
	}
	if((d->mode&0111) && dynldable(fd)){
		dynld = 1;
		goto done1;
	}
	length = d->length;
	code = mallocz(length, 0);
	if(code == nil)
		goto done;

	n = kread(fd, code, length);
	if(n != length) {
		DBG("readmod kread failed on path %s length %d, read n %d\n", path, length, n);
		free(code);
		code = nil;
	}
done:
	if(fd >= 0)
		kclose(fd);
done1:
	if(sync)
		acquire();
	if(m != nil && ans == nil)
		unload(m);
	if(code != nil) {
		ans = parsemod(path, code, length, d);
		free(code);
	}
	else if(dynld){
		kseek(fd, 0, 0);
		ans = newdyncode(fd, path, d);
		kclose(fd);
	}
	free(d);

	/* Free the redirected path if we allocated one */
	if(path_needs_free)
		free((void*)path);

	if(ans != nil)
		READMOD_LOG("readmod: SUCCESS loaded module %s", redirected_path ? redirected_path : path);
	else
		READMOD_LOG("readmod: FAILED to load module %s", redirected_path ? redirected_path : path);
	return ans;
}

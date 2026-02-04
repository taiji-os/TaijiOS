/*
 * Android file system device driver
 * Maps Inferno filesystem to Android storage model
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <android/log.h>

#include "dat.h"
#include "fns.h"
#include "error.h"

#define LOG_TAG "TaijiOS-FS"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/*
 * Android storage paths
 */
static char* internal_storage = nil;	/* /data/data/org.taijos.os/files */
static char* external_storage = nil;	/* /sdcard or similar */

/*
 * Initialize Android storage paths
 * Called from android_main.c during initialization
 */
void
android_fs_init(const char* internal_path, const char* external_path)
{
	if(internal_path != nil) {
		kfree(internal_storage);
		internal_storage = strdup(internal_path);
	}

	if(external_path != nil) {
		kfree(external_storage);
		external_storage = strdup(external_path);
	}

	LOGI("FS paths: internal=%s external=%s",
		internal_storage ? internal_storage : "nil",
		external_storage ? external_storage : "nil");
}

/*
 * Map Inferno path to Android path
 */
static char*
map_path(const char* path)
{
	char* result;
	static char buf[1024];

	/* Absolute paths go to internal storage */
	if(path[0] == '/') {
		/* Check for external storage reference */
		if(strncmp(path, "/sdcard/", 8) == 0) {
			if(external_storage != nil) {
				snprint(buf, sizeof(buf), "%s%s", external_storage, path + 7);
				return buf;
			}
		}

		/* Default to internal storage */
		if(internal_storage != nil) {
			snprint(buf, sizeof(buf), "%s%s", internal_storage, path);
			return buf;
		}

		/* Fallback to direct path */
		snprint(buf, sizeof(buf), "%s", path);
		return buf;
	}

	/* Relative paths stay relative */
	snprint(buf, sizeof(buf), "%s", path);
	return buf;
}

/*
 * Open file
 */
int
kopen(const char* path, int mode)
{
	char* android_path;
	int flags, fd;

	android_path = map_path(path);

	/* Map Inferno mode to POSIX flags */
	switch(mode & 3) {
	case OREAD:
		flags = O_RDONLY;
		break;
	case OWRITE:
		flags = O_WRONLY;
		break;
	case ORDWR:
		flags = O_RDWR;
		break;
	default:
		flags = O_RDONLY;
		break;
	}

	if(mode & OTRUNC)
		flags |= O_TRUNC;
	if(mode & OCREAT)
		flags |= O_CREAT;

	fd = open(android_path, flags, 0666);
	if(fd < 0) {
		LOGE("open failed: %s -> %s: %s", path, android_path, strerror(errno));
		return -1;
	}

	return fd;
}

/*
 * Close file
 */
int
kclose(int fd)
{
	return close(fd);
}

/*
 * Read from file
 */
s32
kread(int fd, void* buf, s32 count)
{
	return read(fd, buf, count);
}

/*
 * Write to file
 */
s32
kwrite(int fd, void* buf, s32 count)
{
	return write(fd, buf, count);
}

/*
 * Seek in file
 */
vlong
kseek(int fd, vlong offset, int whence)
{
	return lseek(fd, offset, whence);
}

/*
 * Create directory
 */
int
kcreate(const char* path, int mode, ulong perm)
{
	char* android_path;
	int result;

	USED(mode);

	android_path = map_path(path);
	result = mkdir(android_path, perm);

	if(result < 0 && errno == EEXIST)
		result = 0;	/* Already exists is OK */

	return result;
}

/*
 * Remove file or directory
 */
int
kremove(const char* path)
{
	char* android_path;
	struct stat st;

	android_path = map_path(path);

	if(stat(android_path, &st) < 0)
		return -1;

	if(S_ISDIR(st.st_mode))
		return rmdir(android_path);
	else
		return unlink(android_path);
}

/*
 * Stat file
 */
int
kstat(const char* path, uchar* buf, int n)
{
	char* android_path;
	struct stat st;

	USED(n);

	android_path = map_path(path);

	if(stat(android_path, &st) < 0)
		return -1;

	/* Fill in Inferno stat buffer */
	/* TODO: Map stat fields properly */

	return 0;
}

/*
 * File system stat
 */
int
kfstat(int fd, uchar* buf, int n)
{
	struct stat st;

	USED(n);

	if(fstat(fd, &st) < 0)
		return -1;

	/* Fill in Inferno stat buffer */
	/* TODO: Map stat fields properly */

	return 0;
}

/*
 * Get current directory
 */
int
kgetwd(char* buf, int n)
{
	if(getcwd(buf, n) == nil)
		return -1;
	return 0;
}

/*
 * Change directory
 */
int
kchdir(const char* path)
{
	char* android_path;

	android_path = map_path(path);
	return chdir(android_path);
}

/*
 * Duplicate file descriptor
 */
int
kdup(int fd1, int fd2)
{
	return dup2(fd1, fd2);
}

/*
 * Pipe creation
 */
int
kpipe(int fd[2])
{
	return pipe(fd);
}

/*
 * File system info
 */
int
kstatfs(char* path, ulong* bsize, uvlong* blocks, uvlong* bfree)
{
	struct statvfs st;
	char* android_path;

	android_path = map_path(path);

	if(statvfs(android_path, &st) < 0)
		return -1;

	if(bsize)
		*bsize = st.f_frsize;
	if(blocks)
		*blocks = st.f_blocks;
	if(bfree)
		*bfree = st.f_bavail;

	return 0;
}

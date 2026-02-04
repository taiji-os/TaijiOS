/*
 * Android network device driver
 * Uses BSD socket API (Android supports standard POSIX sockets)
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#include "dat.h"
#include "fns.h"
#include "error.h"
#include "ip.h"

#define LOG_TAG "TaijiOS-IP"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/*
 * Android network permissions are handled via AndroidManifest.xml
 * No special handling needed here beyond standard socket API
 */

/*
 * Set socket to non-blocking mode
 */
static int
set_nonblock(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL, 0);
	if(flags < 0)
		return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/*
 * Create a socket
 */
int
so_socket(int type, int protocol)
{
	int fd, domain;

	/* Map Inferno types to POSIX */
	switch(type) {
	case AF_INET:
		domain = AF_INET;
		break;
	case AF_INET6:
		domain = AF_INET6;
		break;
	default:
		domain = AF_INET;
		break;
	}

	fd = socket(domain, (protocol == SOCK_STREAM) ? SOCK_STREAM : SOCK_DGRAM, 0);
	if(fd < 0)
		return -1;

	/* Set non-blocking */
	set_nonblock(fd);

	/* Set common options */
	int on = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	return fd;
}

/*
 * Bind socket to address
 */
int
so_bind(int fd, uchar* addr, int addrlen)
{
	struct sockaddr_in sa;

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = *(ushort*)(addr + 2); /* Port is at offset 2 */
	memcpy(&sa.sin_addr, addr + 4, 4);

	return bind(fd, (struct sockaddr*)&sa, sizeof(sa));
}

/*
 * Listen for connections
 */
int
so_listen(int fd, int backlog)
{
	return listen(fd, backlog);
}

/*
 * Accept connection
 */
int
so_accept(int fd, uchar* addr, int* addrlen)
{
	struct sockaddr_in sa;
	socklen_t len = sizeof(sa);
	int nfd;

	nfd = accept(fd, (struct sockaddr*)&sa, &len);
	if(nfd < 0)
		return -1;

	/* Set non-blocking */
	set_nonblock(nfd);

	/* Fill in address */
	if(addr != nil) {
		*(ushort*)addr = sa.sin_family;
		*(ushort*)(addr + 2) = sa.sin_port;
		memcpy(addr + 4, &sa.sin_addr, 4);
		*addrlen = 8;
	}

	return nfd;
}

/*
 * Connect to remote address
 */
int
so_connect(int fd, uchar* addr, int addrlen)
{
	struct sockaddr_in sa;

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = *(ushort*)(addr + 2);
	memcpy(&sa.sin_addr, addr + 4, 4);

	return connect(fd, (struct sockaddr*)&sa, sizeof(sa));
}

/*
 * Send data
 */
long
so_send(int fd, void* data, long len, int flags)
{
	return send(fd, data, len, flags);
}

/*
 * Receive data
 */
long
so_recv(int fd, void* data, long len, int flags)
{
	return recv(fd, data, len, flags);
}

/*
 * Close socket
 */
int
so_close(int fd)
{
	return close(fd);
}

/*
 * Get socket error
 */
int
so_err(int fd)
{
	int error = 0;
	socklen_t len = sizeof(error);
	getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len);
	return error;
}

/*
 * Resolve hostname
 */
int
so_gethostbyname(char* name, uchar* addr)
{
	struct hostent* he;

	he = gethostbyname(name);
	if(he == nil)
		return -1;

	if(he->h_addrtype != AF_INET || he->h_length != 4)
		return -1;

	memcpy(addr, he->h_addr, 4);
	return 0;
}

/*
 * DNS lookup for Android
 */
int
so_gethostbyaddr(char* name, uchar* addr)
{
	struct hostent* he;
	struct in_addr in;

	memcpy(&in.s_addr, addr, 4);
	he = gethostbyaddr(&in, 4, AF_INET);
	if(he == nil)
		return -1;

	strncpy(name, he->h_name, 256);
	return 0;
}

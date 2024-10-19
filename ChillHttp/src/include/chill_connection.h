#pragma once

#include <chill_log.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

// TODO multiplatform
#define WIN32_LEAN_AND_MEAN
#pragma comment(lib, "ws2_32.lib") // Winsock library
#undef TEXT
#include <winsock2.h>
#include <ws2tcpip.h>

typedef enum {
	CONNECTION_STATUS_UNKNOWN	= 0,
	CONNECTION_STATUS_CONNECTED = 1,
	CONNECTION_STATUS_CLOSING	= 2,
	CONNECTION_STATUS_CLOSED	= 3,
	CONNECTION_STATUS_ABORTING	= 4,
	CONNECTION_STATUS_CREATED	= 5,
	CONNECTION_STATUS_WORKING	= 6,
} ConnectionStatus;

typedef struct _cSocket {
	SOCKET socket;
	ConnectionStatus connectionStatus;
} cSocket;

errno_t chill_socket_global_setup();
errno_t chill_getaddrinfo(const char* nodename, const char* port, const struct addrinfo* hints, struct addrinfo** addrinfo);

cSocket* chill_socket(const struct addrinfo* addrinfo);
void chill_socket_free(const cSocket* socket);

errno_t chill_socket_bind(const cSocket* socket, const struct addrinfo* addrinfo);
errno_t chill_socket_listen(const cSocket* socket, int backlog);
cSocket* chill_socket_accept(const cSocket* socket, struct sockaddr* addr, int* addrlen);
errno_t chill_socket_settimeout(const cSocket* socket, size_t timeout);
errno_t chill_socket_select(const cSocket* socket, size_t millisecs);
// TODO errno_t chill_socket_shutdown(const cSocket* socket); // TODO replace shutdown chill task function

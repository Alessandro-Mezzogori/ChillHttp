#include <chill_connection.h>

errno_t chill_socket_global_setup() {
	errno_t result = 0;
	WSADATA wsa;

	LOG_INFO("Initialiasing Winsock");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		result = 1;
		LOG_FATAL("Failed. error code %d", WSAGetLastError());
		goto _cleanup;
	}
	LOG_INFO("Winsock initialized");

_cleanup:
	return result;
}


errno_t chill_getaddrinfo(const char* nodename, const char* port, const struct addrinfo* hints, struct addrinfo** addrinfo) {
	if (getaddrinfo(nodename, port, hints, addrinfo) != 0) {
		LOG_FATAL("getaddrinfo failed with error: %d", WSAGetLastError());
		return 1;
	}

	return 0;
}

cSocket* _chill_socket_alloc() {
	cSocket* sck = malloc(sizeof(cSocket));
	if (sck == NULL) {
		return NULL;
	}

	sck->socket = 0;
	sck->connectionStatus = CONNECTION_STATUS_CREATED;

	return sck;
}

cSocket* chill_socket(const struct addrinfo* addrinfo) {
	cSocket* sck = _chill_socket_alloc();
	if (sck == NULL) {
		return NULL;
	}

	sck->socket = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
	if (sck->socket == INVALID_SOCKET) {
		LOG_FATAL("Could not create socket: %d", WSAGetLastError());
		chill_socket_free(sck);
		return NULL;
	}

	return sck;
}

void chill_socket_free(const cSocket* socket) {
	if (socket != NULL) {
		closesocket(socket->socket);
	}

	free(socket);
}

errno_t chill_socket_bind(const cSocket* socket, const struct addrinfo* addrinfo) {
	if (bind(socket->socket, addrinfo->ai_addr, (int)addrinfo->ai_addrlen) == SOCKET_ERROR) {
		LOG_TRACE("Bind failed with error: %d", WSAGetLastError());
		return 1;
	};

	return 0;
}

errno_t chill_socket_listen(const cSocket* socket, int backlog) {
	if (listen(socket->socket, backlog) == SOCKET_ERROR) {
		LOG_FATAL("Listened failed: %d", WSAGetLastError());
		return 1;
	}

	return 0;
}

cSocket* chill_socket_accept(const cSocket* socket, struct sockaddr* addr, int* addrlen) {
	SOCKET clientSocket = accept(socket->socket, addr, addrlen);
	if(clientSocket == INVALID_SOCKET) {
		LOG_FATAL("Server socket accept failed: %d", WSAGetLastError());
		return NULL;
	}

	cSocket* sck = _chill_socket_alloc();
	sck->socket = clientSocket;
	return sck;
}

errno_t chill_socket_settimeout(const cSocket* socket, size_t timeout) {
	int setsockoptRes = setsockopt(socket->socket, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof(timeout));
	if (setsockopt == SOCKET_ERROR) {
		LOG_ERROR("setsockopt failed: %d", WSAGetLastError());
		return 1;
	}

	return 0;
}

errno_t chill_socket_select(const cSocket* socket, size_t millisecs) {
	size_t secs = millisecs / 1000;
	struct timeval tv = {
		.tv_sec = secs,
		.tv_usec = (millisecs - secs * 1000)
	};
	fd_set fs;
	int messageAvailable = 0;

	FD_ZERO(&fs);
	FD_SET(socket->socket, &fs);
	messageAvailable = select(FD_SETSIZE, &fs, NULL, NULL, &tv);
	if (messageAvailable) {
		return 0;
	}

	return 1;
}

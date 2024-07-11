#pragma once

#include <stdlib.h>
#include <chill_log.h>
#include <chill_connection.h>
#include <chill_lock.h>

#define CHILL_SOCKET_REGISTRY_SIZE 64


typedef struct _ChillSocketRegistry {
	size_t nsocks;
	cSocket* sockets[CHILL_SOCKET_REGISTRY_SIZE];

	ChillLock* _lock;
} ChillSocketRegistry;

errno_t chill_socket_registry_init(ChillSocketRegistry* reg);
void chill_socket_registry_add(cSocket* socket, ChillSocketRegistry* reg);
void chill_socket_registry_remove(cSocket* socket, ChillSocketRegistry* reg);
void chill_socket_registry_free(ChillSocketRegistry* reg);

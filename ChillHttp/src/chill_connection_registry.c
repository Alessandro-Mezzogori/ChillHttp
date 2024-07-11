#include <chill_connection_registry.h>

errno_t chill_socket_registry_init(ChillSocketRegistry* registry) {
	registry->nsocks = 0;
	registry->_lock = lockInit();
	memset(registry->sockets, NULL, CHILL_SOCKET_REGISTRY_SIZE * sizeof(cSocket*));
}

void chill_socket_registry_add(cSocket* socket, ChillSocketRegistry* reg) {
	monitorEnter(reg->_lock);
	for (int i = 0; i < CHILL_SOCKET_REGISTRY_SIZE; i++) {
		if (reg->sockets[i] == NULL) {
			reg->sockets[i] = socket;
			reg->nsocks++;
			break;
		}
	}
	monitorExit(reg->_lock);
}

void chill_socket_registry_remove(cSocket* socket, ChillSocketRegistry* reg) {
	monitorEnter(reg->_lock);
	for (int i = 0; i < CHILL_SOCKET_REGISTRY_SIZE; i++) {
		if (reg->sockets[i]->socket == socket->socket) {
			reg->sockets[i] = NULL;
			reg->nsocks--;
			break;
		}
	}
	monitorExit(reg->_lock);
}

void chill_socket_registry_free(ChillSocketRegistry* reg) {
	lockFree(reg->_lock);
}

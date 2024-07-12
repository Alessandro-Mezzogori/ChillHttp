#include <stdlib.h>
#include <stdio.h>
#include <locale.h>

#include <chill_connection.h>
#include <chill_log.h>
#include <chill_hashtable.h>
#include <chill_config.h>
#include <chill_http.h>
#include <chill_thread.h>
#include <chill_threadpool.h>
#include <chill_task_function.h>
#include <chill_connection_registry.h>

#define WinsockInitializedError 1
#define SocketCreationError 2
#define ListenError 3

void freeMainThreadData(PMTData data) {
	if (data == NULL) {
		return;
	}

	const size_t maxConcurrentThreads = data->config.maxConcurrentThreads;
	closesocket(data->serverSocket);
	free(data);
}

typedef struct _SocketRegistryThreadData {
	ChillSocketRegistry* registry;
	ChillThreadPool* threadPool;
	Config* config;
	int exit;
} SocketRegistryThreadData;

void socketRegistryFunction(void* data) {
	SocketRegistryThreadData* tdata = (SocketRegistryThreadData*)data;
	ChillSocketRegistry* reg = tdata->registry;

	while (!tdata->exit) {
		for (int i = 0; i < CHILL_SOCKET_REGISTRY_SIZE; i++) {
			monitorEnter(reg->_lock);
			cSocket* socket = reg->sockets[i];
			if (
				socket != NULL 
				&& (
					socket->connectionStatus == CONNECTION_STATUS_CONNECTED 
					|| socket->connectionStatus == CONNECTION_STATUS_CREATED
					) 
				&& chill_socket_select(socket, 10) == 0
			) {
				LOG_DEBUG("Task context creation");
				TaskContext* taskContext = malloc(sizeof(TaskContext));
				if (taskContext == NULL) {
					LOG_ERROR("TaskContext allocation failed");
					continue;
				}

				if (socket->connectionStatus == CONNECTION_STATUS_CREATED) {
					socket->connectionStatus = CONNECTION_STATUS_CONNECTED;
				}

				taskContext->config = tdata->config;
				taskContext->httpcontext.connectionData = socket;
				taskContext->httpcontext.isActive = TRUE;
				taskContext->registry = reg;

				ChillTaskInit taskinit = {
					.data = taskContext,
					.work = task_function,
				};
				ChillTask* task = chill_task_create(tdata->threadPool, &taskinit);
				chill_task_submit(task);
			}
			monitorExit(reg->_lock);
		}
	}
}

int main() { 
	ChillThreadPoolInit init = {
		.max_thread = 2,
		.min_thread = 1,
	};
	ChillThreadPool* pool = NULL;
	Config config;
	int cleanup = 0;
	errno_t err = 0;
	PMTData mtData = NULL;
	struct addrinfo hints;
	struct addrinfo *addrinfo = NULL;	
	SOCKET serverSocket = NULL;
	ChillSocketRegistry socketRegistry;
	SocketRegistryThreadData socketRegistryThreadData = {
		.registry = &socketRegistry,
		.exit = 0,
		.config = &config,
	};
	ChillThread* registryThread = NULL;
	ChillThreadInit registryThreadInit = {
		.data = (void*) &socketRegistryThreadData,
		.delayStart = FALSE,
		.work = socketRegistryFunction
	};

	setlocale(LC_ALL, "");
	LOG_INFO("Loading configuration");
	loadConfig(&config);

	LOG_INFO("Threadpool initialization");
	err = chill_threadpool_init(&pool, &init);
	if (err != 0) {
		LOG_FATAL("Threadpool initialization failed, err: %d", err);
		goto _cleanup;
	}
	cleanup = 2; // threadpool initialized
	LOG_INFO("Threadpool initialized");

	LOG_INFO("Socket registry initialization");
	chill_socket_registry_init(&socketRegistry);
	socketRegistryThreadData.registry = &socketRegistry;
	socketRegistryThreadData.threadPool = pool;
	socketRegistryThreadData.config = &config;

	LOG_INFO("Starting socket registry thread");
	err = chill_thread_init(&registryThreadInit, &registryThread);
	if (err != 0) {
		LOG_FATAL("Socket registry thread failed, err: %d", err);
		goto _cleanup;
	}
	cleanup = 3;

	if (chill_socket_global_setup() != 0) {
		goto _cleanup;
	}
	cleanup = 4; // winsock startup

	LOG_INFO("Initialiasing port");
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	if (chill_getaddrinfo(NULL, config.port, &hints, &addrinfo) != 0) {
		goto _cleanup;
	}
	cleanup = 5; // get addrinfo

	LOG_INFO("Main thread data setup...");
	mtData = malloc(sizeof(MTData));
	if(mtData == NULL) {
		LOG_FATAL("Malloc failed");
		goto _cleanup;
	}
	mtData->config = config;
	mtData->isRunning = TRUE;
	mtData->serverSocket = NULL;
	cleanup = 6; // main thread data allocated
	LOG_INFO("Main thread setup done");

	LOG_INFO("Server socket creation...");
	mtData->serverSocket = chill_socket(addrinfo);
	serverSocket = mtData->serverSocket;
	if (serverSocket == NULL) {
		goto _cleanup;
	}
	cleanup = 7; // server socket created
	LOG_INFO("Server socket created");

	LOG_INFO("Server socket binding");
	if(chill_socket_bind(serverSocket, addrinfo) != 0) {
		LOG_FATAL("Bind failed with error");
		goto _cleanup;
	}
	cleanup = 8; // server socket bind

	LOG_INFO("Server socket listening");
	if (chill_socket_listen(serverSocket, config.maxConcurrentThreads) != 0) {
		LOG_FATAL("Listened failed");
		goto _cleanup;
	}

	LOG_INFO("Setup successfull, ready to accept connections");
	while (TRUE) {
		int loop_cleanup = 0;
		TaskContext* taskContext = NULL;
		cSocket* clientSocket = chill_socket_accept(serverSocket, NULL, NULL);
		if(clientSocket == NULL) {
			LOG_FATAL("Server socket accept failed");
			goto _loop_cleanup;
		}
		loop_cleanup = 1;
		LOG_INFO("Server socket accept successfull socket %d", clientSocket->socket);

		LOG_DEBUG("Clien socket %d setup options", clientSocket->socket);
		if(chill_socket_settimeout(clientSocket, config.recvTimeout) != 0) {
			LOG_ERROR("setsockopt failed");
			goto _loop_cleanup;
		}

		LOG_INFO("Adding socket %d to socket registry", clientSocket->socket);
		chill_socket_registry_add(clientSocket, &socketRegistry);

		/*
		LOG_DEBUG("Task context creation");
		taskContext = malloc(sizeof(TaskContext));
		if (taskContext == NULL) {
			LOG_ERROR("TaskContext allocation failed");
			goto _loop_cleanup;
		}
		loop_cleanup = 2;

		clientSocket->connectionStatus = CONNECTION_STATUS_CONNECTED;
		taskContext->config = &config;
		taskContext->httpcontext.connectionData = clientSocket;
		taskContext->httpcontext.isActive = TRUE;

		ChillTaskInit taskinit = {
			.data = taskContext,
			.work = task_function,
		};
		ChillTask* task = chill_task_create(pool, &taskinit);
		chill_task_submit(task);
		*/

		// TODO error to client
		// TODO registry for all open connections
		// TODO cleanup of task context
		// TODO cleanup of chill task
		// TODO cleanup of socket

		continue;
	_loop_cleanup:
		switch (loop_cleanup) {
			case 2:
				free(taskContext);
			case 1:
				chill_socket_free(clientSocket);
			default:
				break;
		}
		break;
	}

_cleanup: 
	switch (cleanup) {
		case 7:
			chill_socket_free(mtData->serverSocket);
		case 6:
			mtData->isRunning = FALSE;
			freeMainThreadData(mtData);
		case 5:
			freeaddrinfo(addrinfo);
		case 4:
			WSACleanup();
		case 3:
			chill_thread_join(registryThread, -1);
			chill_thread_cleanup(&registryThread);
		case 2:
			chill_threadpool_free(pool);
		case 1:
			chill_socket_registry_free(&socketRegistry);
		default:
			break;
	}

	closesocket(serverSocket);

	LOG_INFO("Cleanup");
	// TODO cleanup of test tasks

	LOG_INFO("Shutting down...");
	LOG_FLUSH();
	return 0;
}

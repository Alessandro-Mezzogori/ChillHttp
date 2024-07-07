#define WIN32_LEAN_AND_MEAN

#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>

#include <chill_log.h>
#include <chill_hashtable.h>
#include <chill_config.h>
#include <chill_http.h>
#include <chill_thread_old.h>
#include <chill_thread.h>
#include <chill_threadpool.h>

#pragma comment(lib, "ws2_32.lib") // Winsock library

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

int main() { 
	ChillThreadPoolInit init = {
		.max_thread = 2,
		.min_thread = 1,
	};
	ChillThreadPool* pool;
	Config config;
	struct WSAData wsa;
	int cleanup = 0;
	errno_t err = 0;
	PMTData mtData = NULL;
	struct addrinfo hints;
	struct addrinfo *addrinfo = NULL;	
	SOCKET serverSocket = NULL;

	setlocale(LC_ALL, "");
	LOG_INFO("Loading configuration");
	loadConfig(&config);

	LOG_INFO("Threadpool initialization");
	err = chill_threadpool_init(&pool, &init);
	if (err != 0) {
		LOG_FATAL("Threadpool initialization failed, err: %d", err);
		goto _cleanup;
	}
	cleanup = 1; // threadpool initialized
	LOG_INFO("Threadpool initialized");

	LOG_INFO("Initialiasing Winsock");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		LOG_FATAL("Failed. error code %d", WSAGetLastError());
		goto _cleanup;
	}
	cleanup = 2; // winsock startup
	LOG_INFO("Winsock initialized");

	LOG_INFO("Initialiasing port");
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, config.port, &hints, &addrinfo) != 0) {
		LOG_FATAL("getaddrinfo failed with error: %d", WSAGetLastError());
		goto _cleanup;
	}
	cleanup = 3; // get addrinfo

	LOG_INFO("Main thread data setup...");
	mtData = malloc(sizeof(MTData));
	if(mtData == NULL) {
		LOG_FATAL("Malloc failed");
		goto _cleanup;
	}
	mtData->config = config;
	mtData->isRunning = TRUE;
	cleanup = 4; // main thread data allocated
	LOG_INFO("Main thread setup done");

	LOG_INFO("Server socket creation...");
	mtData->serverSocket = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
	serverSocket = mtData->serverSocket;
	if (serverSocket == INVALID_SOCKET) {
		LOG_FATAL("Could not create socket: %d", WSAGetLastError());
		goto _cleanup;
	}
	cleanup = 5; // server socket created
	LOG_INFO("Server socket created");

	LOG_INFO("Server socket binding");
	if(bind(serverSocket, addrinfo->ai_addr, (int)addrinfo->ai_addrlen) == SOCKET_ERROR) {
		LOG_FATAL("Bind failed with error: %d", WSAGetLastError());
		goto _cleanup;
	}
	cleanup = 6; // server socket bind

	LOG_INFO("Server socket listening");
	if (listen(serverSocket, config.maxConcurrentThreads) == SOCKET_ERROR) {
		LOG_FATAL("Listened failed: %d", WSAGetLastError());
		goto _cleanup;
	}

	LOG_INFO("Setup successfull, ready to accept connections");
	while (TRUE) {
		int loop_cleanup = 0;
		TaskContext* taskContext = NULL;
		SOCKET clientSocket = accept(serverSocket, NULL, NULL);
		if(clientSocket == INVALID_SOCKET) {
			// TODO handle error
			LOG_FATAL("Server socket accept failed: %d", WSAGetLastError());
			goto _loop_cleanup;
		}
		loop_cleanup = 1;
		LOG_INFO("Server socket accept successfull socket %d", clientSocket);

		LOG_DEBUG("Clien socket %d setup options", clientSocket);
		int setsockoptRes = setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&config.recvTimeout, sizeof(config.recvTimeout));
		if(setsockoptRes == SOCKET_ERROR) {
			LOG_ERROR("setsockopt failed: %d", WSAGetLastError());
			goto _loop_cleanup;
		}

		LOG_DEBUG("Task context creation");
		taskContext = malloc(sizeof(TaskContext));
		if (taskContext == NULL) {
			LOG_ERROR("TaskContext allocation failed");
			goto _loop_cleanup;
		}
		loop_cleanup = 2;

		taskContext->config = &config;
		taskContext->httpcontext.connectionData.connectionStatus = CONNECTION_STATUS_CONNECTED;
		taskContext->httpcontext.socket = clientSocket;
		taskContext->httpcontext.isActive = TRUE;

		ChillTaskInit taskinit = {
			.data = taskContext,
			.work = task_function,
		};
		ChillTask* task = chill_task_create(pool, &taskinit);
		chill_task_submit(task);

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
				closesocket(clientSocket);
			default:
				break;
		}
		break;
	}

_cleanup: 
	switch (cleanup) {
		case 4:
			mtData->isRunning = FALSE;
			freeMainThreadData(mtData);
		case 3:
			freeaddrinfo(addrinfo);
		case 2:
			WSACleanup();
		case 1:
			chill_threadpool_free(pool);
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

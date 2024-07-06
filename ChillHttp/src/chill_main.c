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

#define task_num 10000

void freeChildThreadDescriptor(PCTD child) {
	if (child->hThread != NULL) {
		CloseHandle(child->hThread);
		child->hThread = NULL;
	}

	free(child->pdata);
	child->pdata = NULL;

	child->dwThreadId = 0;
}

void freeMainThreadData(PMTData data) {
	if (data == NULL) {
		return;
	}

	const size_t maxConcurrentThreads = data->config.maxConcurrentThreads;
	PCTD childs = data->childs;
	if (childs != NULL) {
		for(int i = 0; i < maxConcurrentThreads; i++) {
			freeChildThreadDescriptor(&childs[i]);
		}
	}

	free(data->childs);
	closesocket(data->serverSocket);
	free(data);
}

DWORD cleanupThreadFunction(void* lpParam) {
	PMTData pdata = (PMTData)lpParam;

	LOG_TRACE("Cleanup thread initialized");

	const size_t maxConcurrentThreads = pdata->config.maxConcurrentThreads;
	while (pdata->isRunning) {
		Sleep(1000);

		for(int i = 0; i < maxConcurrentThreads; i++) {
			PCTD child = &pdata->childs[i];

			if (child->hThread != NULL) {
				DWORD exitCode = 0;

				if(GetExitCodeThread(child->hThread, &exitCode) != 0) {
					if (exitCode != STILL_ACTIVE) {
						LOG_TRACE("Thread {%d} exited with code {%d}", i, exitCode);
						LOG_TRACE("Cleanup thread {%d}");

						freeChildThreadDescriptor(child);

						pdata->activeThreadCount -= 1;
						LOG_TRACE("Active thread count: %d", pdata->activeThreadCount);
					}
				}
			}
		}
	}

	return 0;
}

typedef struct Test {
	int num;
} T;

void test_work(void* context) {
	T* test = (T*) context;

	//LOG_INFO("Data %d", test->num);
	LOG_INFO("La topi puzza");
}

int main() { 
	ChillThreadPool* pool;
	T testData[task_num]; 
	ChillTask* tasks[task_num];
	ChillTaskInit taskinit[task_num];

	for (int i = 0; i < task_num; i++) {
		testData[i].num = i;
	}

	LOG_INFO("Threadpool initialization");
	ChillThreadPoolInit init = {
		.max_thread = 2,
		.min_thread = 1,
	};
	chill_threadpool_init(&pool, &init);

	LOG_INFO("Task creation");
	for (int i = 0; i < task_num; i++) {
		taskinit[i].data = &testData[i];
		taskinit[i].work = test_work;

		tasks[i] = chill_task_create(pool, &taskinit[i]);
		if (tasks[i] == NULL) {
			LOG_ERROR("Task creation failed %d", i);
			break;
		}
	}

	LOG_INFO("Task submit");
	for (int i = 0; i < task_num; i++) {
		chill_task_submit(tasks[i]);
	}

	LOG_INFO("Wait tasks");
	chill_task_wait_multiple(tasks, task_num);

	LOG_INFO("Cleanup");
	// TODO cleanup of test tasks
	chill_threadpool_free(pool);

	return 0;


	setlocale(LC_ALL, "");

	Config config;
	loadConfig(&config);

	struct WSAData wsa;
	LOG_INFO("Initialiasing Winsock");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		LOG_FATAL("Failed. error code %d", WSAGetLastError());
		return WinsockInitializedError;
	}

	LOG_INFO("Initialiasing port");
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo *result = NULL;	
	if (getaddrinfo(NULL, config.port, &hints, &result) != 0) {
		LOG_FATAL("getaddrinfo failed with error: %d", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	LOG_INFO("Main thread data setup...");
	PMTData mtData = malloc(sizeof(MTData));
	if(mtData == NULL) {
		LOG_FATAL("Malloc failed");
		WSACleanup();
		return 1;
	}

	mtData->config = config;
	mtData->isRunning = TRUE;
	mtData->activeThreadCount = 0;
	mtData->childs = calloc(config.maxConcurrentThreads, sizeof(CTD));
	if(mtData->childs == NULL) {
		LOG_FATAL("Calloc failed");
		freeMainThreadData(mtData);
		WSACleanup();
		return 1;
	}

	LOG_INFO("Server socket creation...");
	mtData->serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	SOCKET serverSocket = mtData->serverSocket;
	if (serverSocket == INVALID_SOCKET) {
		LOG_FATAL("Could not create socket: %d", WSAGetLastError());
		freeMainThreadData(mtData);
		WSACleanup();
		return SocketCreationError;
	}
	LOG_INFO("Server socket created");

	LOG_INFO("Server socket binding");
	if(bind(serverSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
		LOG_FATAL("Bind failed with error: %d", WSAGetLastError());
		freeMainThreadData(mtData);
		WSACleanup();
		return SocketCreationError;
	}

	LOG_DEBUG("[free] main address");
	freeaddrinfo(result);

	LOG_INFO("Server socket listening");
	if (listen(serverSocket, config.maxConcurrentThreads) == SOCKET_ERROR) {
		LOG_FATAL("Listened failed: %d", WSAGetLastError());
		freeMainThreadData(mtData);
		WSACleanup();
		return ListenError;
	}

	LOG_INFO("Cleanup thread instantiation");
	HANDLE cleanupThread = CreateThread(NULL, 0, cleanupThreadFunction, mtData, 0, NULL);
	if(cleanupThread == NULL) {
		LOG_FATAL("CreateThread failed: %d", WSAGetLastError());
		freeMainThreadData(mtData);
		WSACleanup();
		return 1;
	}

	LOG_INFO("Setup successfull, ready to accept connections");
	PCTD childs = mtData->childs;
	while (TRUE) {
		SOCKET clientSocket = accept(serverSocket, NULL, NULL);
		if(clientSocket == INVALID_SOCKET) {
			// TODO handle error
			LOG_FATAL("Accept failed: %d", WSAGetLastError());
			break;
		}
		LOG_INFO("Accept successfull socket %d", clientSocket);

		LOG_DEBUG("Setup socket %d options", clientSocket);
		int setsockoptRes = setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&config.recvTimeout, sizeof(config.recvTimeout));
		if(setsockoptRes == SOCKET_ERROR) {
			LOG_ERROR("setsockopt failed: %d", WSAGetLastError());
			closesocket(clientSocket);
			continue;
		}

		// find first free thread
		LOG_INFO("Finding free thread");
		int availableThreadIndex = -1;
		for(int i = 0; i < config.maxConcurrentThreads; i++) {
			PCTD child = &childs[i];

			if (child != NULL && child->hThread == NULL) {
				availableThreadIndex = i;
				break;
			}
		}

		if (availableThreadIndex >= 0) {
			LOG_TRACE("Creating thread %d", availableThreadIndex);
			PCTD threadDescriptor = &childs[availableThreadIndex];

			PSOCKTD pdata = (PSOCKTD)malloc(sizeof(SOCKTD));
			if (pdata == NULL) {
				LOG_FATAL("Malloc failed");
				break;
			}

			pdata->threadId = availableThreadIndex;
			pdata->socket = clientSocket;
			pdata->isActive = TRUE;
			pdata->connectionData.connectionStatus = CONNECTION_STATUS_CONNECTED;

			// idk if it's the best way to do it, for now it's fine
			// TODO find better way to pass env data to thread
			pdata->pmtData = mtData;

			threadDescriptor->hThread = CreateThread(NULL, 0, threadFunction, pdata, 0, &threadDescriptor->dwThreadId);
			threadDescriptor->pdata = pdata;

			mtData->activeThreadCount += 1;
			LOG_TRACE("Active thread count: %d", mtData->activeThreadCount);
		}
		else {
			LOG_ERROR("No free threads");

			// TODO error to client
			closesocket(clientSocket);
		}
	}

	LOG_INFO("Cleaning up");
	for(int i = 0; i < config.maxConcurrentThreads; i++) {
		HANDLE hThread = childs[i].hThread;
		if (hThread != NULL) {
			// TODO handle timeout on wait for single thread  
			WaitForSingleObject(hThread, INFINITE);
		}
	}

	mtData->isRunning = FALSE;
	WaitForSingleObject(cleanupThread, INFINITE);
	CloseHandle(cleanupThread);

	closesocket(serverSocket);
	WSACleanup();

	freeMainThreadData(mtData);

	LOG_INFO("Shutting down...");
	LOG_FLUSH();
	return 0;
}

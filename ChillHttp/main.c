#define WIN32_LEAN_AND_MEAN

#include "log/log.h"
#include "hashtable.h"
#include "http.h"
#include "thread.h"
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>

#pragma comment(lib, "ws2_32.lib") // Winsock library

#define WinsockInitializedError 1
#define SocketCreationError 2
#define ListenError 3

#define DEFAULT_PORT "27015"
#define MAX_CONCURRENT_THREADS 5


void freeMainThreadData(PMTData data) {
	if (data == NULL) {
		return;
	}

	if(data->hThreadArray != NULL) {
		for (int i = 0; i < MAX_CONCURRENT_THREADS; i++) {
			if (data->hThreadArray[i] != NULL) {
				CloseHandle(data->hThreadArray[i]);
			}
		}
	}

	if(data->pdataThreadArray != NULL) {
		for (int i = 0; i < MAX_CONCURRENT_THREADS; i++) {
			free(data->pdataThreadArray[i]);
		}
	}

	closesocket(data->serverSocket);
	free(data->dwThreadIdArray);
	free(data->hThreadArray);
	free(data->pdataThreadArray);
	free(data->servingFolder);
	free(data);
}

DWORD cleanupThreadFunction(void* lpParam) {
	PMTData pdata = (PMTData)lpParam;

	LOG_TRACE("Cleanup thread initialized");

	while (pdata->isRunning) {
		Sleep(1000);

		for(int i = 0; i < MAX_CONCURRENT_THREADS; i++) {
			if (pdata->hThreadArray[i] != NULL) {
				DWORD exitCode = 0;
				if(GetExitCodeThread(pdata->hThreadArray[i], &exitCode) != 0) {
					if (exitCode != STILL_ACTIVE) {
						LOG_TRACE("Thread {%d} exited with code {%d}", i, exitCode);
						LOG_TRACE("Cleanup thread {%d}");
						CloseHandle(pdata->hThreadArray[i]);

						pdata->hThreadArray[i] = NULL;
						pdata->dwThreadIdArray[i] = 0;
						free(pdata->pdataThreadArray[i]);

						pdata->activeThreadCount -= 1;
						LOG_TRACE("Active thread count: %d", pdata->activeThreadCount);
					}
				}
			}
		}
	}

	return 0;
}


int main() { 
	setlocale(LC_ALL, "");

	struct WSAData wsa;
	LOG_INFO("Initialiasing Winsock");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		LOG_FATAL("Failed. error code %d", WSAGetLastError());
		return WinsockInitializedError;
	}

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo *result = NULL;	
	if (getaddrinfo(NULL, DEFAULT_PORT, &hints, &result) != 0) {
		LOG_FATAL("getaddrinfo failed with error: %d", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	PMTData mtData = malloc(sizeof(MTData));
	if(mtData == NULL) {
		LOG_FATAL("Malloc failed");
		WSACleanup();
		return 1;
	}

	char SERVING_FOLDER[] = "D:\\Projects\\ChillHttp\\ChillHttp\\wwwroot";

	size_t servingFolderLength = strlen(SERVING_FOLDER);
	mtData->servingFolder = (char*) calloc(servingFolderLength + 1, sizeof(char));
	strcpy_s(mtData->servingFolder, servingFolderLength + 1, SERVING_FOLDER); // could use memcpy here

	mtData->isRunning = TRUE;
	mtData->activeThreadCount = 0;
	mtData->dwThreadIdArray = calloc(MAX_CONCURRENT_THREADS, sizeof(DWORD));
	mtData->hThreadArray= calloc(MAX_CONCURRENT_THREADS, sizeof(HANDLE));
	mtData->pdataThreadArray= calloc(MAX_CONCURRENT_THREADS, sizeof(PSOCKTD));

	mtData->serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	SOCKET serverSocket = mtData->serverSocket;
	if (serverSocket == INVALID_SOCKET) {
		LOG_FATAL("Could not create socket: %d", WSAGetLastError());
		freeMainThreadData(mtData);
		WSACleanup();
		return SocketCreationError;
	}
	LOG_INFO("Server socket created");

	if(bind(serverSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
		LOG_FATAL("Bind failed with error: %d", WSAGetLastError());
		freeMainThreadData(mtData);
		WSACleanup();
		return SocketCreationError;
	}

	freeaddrinfo(result);

	DWORD* dwThreadIdArray = mtData->dwThreadIdArray;
	HANDLE* hThreadArray = mtData->hThreadArray;
	PSOCKTD* pdataThreadArray = mtData->pdataThreadArray;

	if (listen(serverSocket, MAX_CONCURRENT_THREADS) == SOCKET_ERROR) {
		LOG_FATAL("Listened failed: %d", WSAGetLastError());
		freeMainThreadData(mtData);
		WSACleanup();
		return ListenError;
	}

	HANDLE cleanupThread = CreateThread(NULL, 0, cleanupThreadFunction, mtData, 0, NULL);
	if(cleanupThread == NULL) {
		LOG_FATAL("CreateThread failed: %d", WSAGetLastError());
		freeMainThreadData(mtData);
		WSACleanup();
		return 1;
	}

	int counter = 0;
	while (TRUE) {
		SOCKET clientSocket = accept(serverSocket, NULL, NULL);
		if(clientSocket == INVALID_SOCKET) {
			// TODO handle error
			LOG_FATAL("Accept failed: %d", WSAGetLastError());
			break;
		}

		// find first free thread
		int availableThreadIndex = -1;
		for(int i = 0; i < MAX_CONCURRENT_THREADS; i++) {
			if (hThreadArray[i] == NULL) {
				availableThreadIndex = i;
				break;
			}
		}

		if (availableThreadIndex >= 0) {
			LOG_TRACE("Creating thread %d", availableThreadIndex);
			PSOCKTD pdata = (PSOCKTD)malloc(sizeof(SOCKTD));
			if (pdata == NULL) {
				LOG_FATAL("Malloc failed");

				// Sets counter to 2 to break outer loop
				counter = 2;
				break;
			}

			pdata->threadId = availableThreadIndex;
			pdata->socket = clientSocket;
			pdata->isActive = TRUE;

			// idk if it's the best way to do it, for now it's fine
			// TODO find better way to pass env data to thread
			pdata->pmtData = mtData;

			hThreadArray[availableThreadIndex] = CreateThread(NULL, 0, threadFunction, pdata, 0, &dwThreadIdArray[availableThreadIndex]);
			mtData->activeThreadCount += 1;
			LOG_TRACE("Active thread count: %d", mtData->activeThreadCount);
			counter++;
		}
		else {
			LOG_ERROR("No free threads");
		}

		if (counter >= 2) {
			break;
		}
	}

	LOG_FLUSH();

	for(int i = 0; i < MAX_CONCURRENT_THREADS; i++) {
		if (hThreadArray[i] != NULL) {
			WaitForSingleObject(hThreadArray[i], INFINITE);
		}
	}

	mtData->isRunning = FALSE;
	WaitForSingleObject(cleanupThread, INFINITE);
	CloseHandle(cleanupThread);

	closesocket(serverSocket);
	WSACleanup();

	freeMainThreadData(mtData);

	return 0;
}

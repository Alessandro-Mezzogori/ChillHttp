#define WIN32_LEAN_AND_MEAN

#include "log/log.h"
#include "hashtable.h"
#include "http.h"
#include <windows.h>
#include <winsock2.h>
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

typedef struct SocketThreadData {
	int threadId;
	SOCKET socket;
	BOOL isActive;
} SOCKTD, *PSOCKTD;

typedef struct MainThreadData {
	SOCKET serverSocket;
	DWORD dwThreadIdArray[MAX_CONCURRENT_THREADS];
	HANDLE hThreadArray[MAX_CONCURRENT_THREADS];
	PSOCKTD pdataThreadArray[MAX_CONCURRENT_THREADS];
	BOOL isRunning; 
	unsigned int activeThreadCount;
} MTData, *PMTData;

void freeMainThreadData(PMTData data) {
	if (data != NULL) {
		if (data->serverSocket != NULL) {
			closesocket(data->serverSocket);
		}

		for(int i = 0; i < MAX_CONCURRENT_THREADS; i++) {
			if (data->hThreadArray[i] != NULL) {
				CloseHandle(data->hThreadArray[i]);
			}

			if (data->pdataThreadArray[i] != NULL) {
				free(data->pdataThreadArray[i]);
			}
		}

		free(data);
	}
}

DWORD cleanupThreadFunction(void *lpParam) {
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

DWORD threadFunction(void *lpParam) {
	PSOCKTD pdata = (PSOCKTD)lpParam;
	int threadId = pdata->threadId;
	SOCKET socket = pdata->socket;

	LOG_INFO("Thread {%d} socket: {%d} initiliazed", threadId, socket);

	int connectionResult = 0;
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	do {
		connectionResult = recv(socket, buffer, sizeof(buffer), 0);
		if (connectionResult > 0) {
			LOG_INFO("Socket {%d} bytes received: %d", socket, connectionResult);
			LOG_INFO("Socket {%d}\r\nMessage: %s", socket, &buffer);

			HttpRequest* request = parseHttpRequest(buffer);

			freeHttpRequest(request);

			static char* headersBuffer =
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: text/html\r\n";

			static char* contentBuffer =
				"<html>\r\n"
				"<body>\r\n"
				"<h1>Hello world</h1>\r\n"
				"</body>\r\n"
				"</html>\r\n";

			size_t contentLength = strlen(contentBuffer);
			char contentLengthBuffer[64];
			memset(contentLengthBuffer, 0, sizeof(contentLengthBuffer));
			sprintf_s(contentLengthBuffer, sizeof(contentLengthBuffer), "Content-Length: %zu\r\n", contentLength);

			char responseBuffer[1024];
			memset(responseBuffer, 0, sizeof(responseBuffer));
			strcat_s(responseBuffer, sizeof(responseBuffer), headersBuffer);
			strcat_s(responseBuffer, sizeof(responseBuffer), contentLengthBuffer);
			strcat_s(responseBuffer, sizeof(responseBuffer), "\r\n");
			strcat_s(responseBuffer, sizeof(responseBuffer), contentBuffer);

			size_t responseBufferLength = strlen(responseBuffer);
			int sendResult = send(socket, responseBuffer, responseBufferLength, 0);
			if(sendResult == SOCKET_ERROR) {
				LOG_FATAL("Socket {%d} send failed: %d", socket, WSAGetLastError());
				closesocket(socket);
				WSACleanup();
				return 1;
			}

			LOG_INFO("Socket {%d} bytes sent: %d", socket, sendResult);
			LOG_INFO("Socket:{%d} Message:\r\n%s", socket, responseBuffer);
		}
		else if (connectionResult == 0) {
			LOG_INFO("Socket {%d} connection closing...", socket);
		}
		else {
			LOG_FATAL("Socket {%d} cecv failed: %d", socket, WSAGetLastError());
			closesocket(socket);
			WSACleanup();
			return 1;
		}
	} while (connectionResult > 0);

	if (shutdown(socket, SD_SEND) == SOCKET_ERROR) {
		LOG_FATAL("Socket {%d} Shutdown failed: %d", socket, WSAGetLastError());
		closesocket(socket);
		WSACleanup();
		return 1;
	}

	LOG_TRACE("Closing thread {%d} socket {%d}", threadId, socket);
	closesocket(socket);

	pdata->isActive = FALSE;
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

	mtData->isRunning = TRUE;
	mtData->activeThreadCount = 0;
	memset(mtData->dwThreadIdArray, 0, sizeof(DWORD)*MAX_CONCURRENT_THREADS);
	memset(mtData->hThreadArray, 0, sizeof(HANDLE)*MAX_CONCURRENT_THREADS);
	memset(mtData->pdataThreadArray, 0, sizeof(PSOCKTD)*MAX_CONCURRENT_THREADS);

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

	for(int i = 0; i < MAX_CONCURRENT_THREADS; i++) {
		if (pdataThreadArray[i] != NULL) {
			LOG_DEBUG("Freeing thread data {%d}", i);
			free(pdataThreadArray[i]);
		}
	}

	closesocket(serverSocket);
	WSACleanup();

	freeMainThreadData(mtData);

	return 0;
}

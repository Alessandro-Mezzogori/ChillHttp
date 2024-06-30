#pragma once

#include <winsock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <chill_log.h>
#include <chill_router.h>
#include <chill_http.h>
#include <chill_config.h>
#include <chill_pipeline.h>
#include <chill_connection.h>

typedef struct MainThreadData MTData, *PMTData;
typedef struct SocketThreadData SOCKTD, *PSOCKTD;
typedef struct ChildThreadDescriptor CTD, *PCTD;


struct SocketThreadData {
	int threadId;
	SOCKET socket;
	bool isActive;
	ConnectionData connectionData;
	PMTData pmtData;
};

struct ChildThreadDescriptor {
	DWORD dwThreadId;
	HANDLE hThread;
	PSOCKTD pdata;
};

struct MainThreadData {
	SOCKET serverSocket;
	PCTD childs;
	BOOL isRunning;
	unsigned int activeThreadCount;
	Config config;
};

DWORD threadFunction(void* lpParam);

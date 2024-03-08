#pragma once

#include "log/log.h"
#include "router.h"
#include "http.h"
#include "config.h"
#include <winsock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef struct MainThreadData MTData, *PMTData;
typedef struct SocketThreadData SOCKTD, *PSOCKTD;

typedef enum {
	CONNECTION_STATUS_UNKNOWN,
	CONNECTION_STATUS_CONNECTED,
	CONNECTION_STATUS_CLOSING,
	CONNECTION_STATUS_CLOSED
} ConnectionStatus;

struct SocketThreadData {
	int threadId;
	SOCKET socket;
	bool isActive;
	ConnectionStatus connectionStatus;
	PMTData pmtData;
};

struct MainThreadData {
	SOCKET serverSocket;
	DWORD* dwThreadIdArray;
	HANDLE* hThreadArray;
	PSOCKTD* pdataThreadArray;
	BOOL isRunning;
	unsigned int activeThreadCount;
	Config config;
};

DWORD threadFunction(void* lpParam);

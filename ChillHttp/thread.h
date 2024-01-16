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

struct SocketThreadData {
	int threadId;
	SOCKET socket;
	bool isActive;
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

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
typedef struct _HttpContext HttpContext;
typedef struct _TaskContext TaskContext;

struct _HttpContext {
	SOCKET socket;
	bool isActive;
	ConnectionData connectionData;
};

struct _TaskContext {
	HttpContext httpcontext;
	const Config* config;
};

struct MainThreadData {
	SOCKET serverSocket;
	BOOL isRunning;
	Config config;
};

void task_function(void* lpParam);

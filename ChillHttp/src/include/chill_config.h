#pragma once

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <chill_lua.h>

#define MAX_CONFIG_LINE_LEN 256
#define CHILL_CONFIG_FILEPATH_LEN 256
#define MAX_SERVING_FOLDER_LEN 256
#define MAX_CONFIG_PORT_LEN 6

typedef struct _Config {
	// ##### SERVING_FOLDER #####
	char servingFolder[MAX_SERVING_FOLDER_LEN];
	size_t servingFolderLen;

	// ##### PORT #####
	char port[MAX_CONFIG_PORT_LEN];

	// ##### CONNECTION #####
	size_t recvTimeout;

	// ##### LOGGING #####

	// ##### THREADS #####
	int maxConcurrentThreads;

	// ##### PIPELINE #####
	char pipelineFilePath[CHILL_CONFIG_FILEPATH_LEN];
	char routesFilePath[CHILL_CONFIG_FILEPATH_LEN];

	// ##### CACHE #####

	// ##### SECURITY #####

	// ##### SSL #####


} Config;

errno_t chill_load_config(const char* str, Config* config);


#pragma once

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <chill_lua.h>

#define CONFIG_PATH_TEXT "conf.txt"
#define CONFIG_PATH_LUA "conf.lua"
#define MAX_CONFIG_LINE_LEN 256
#define MAX_CONFIG_PORT_LEN 6

typedef struct {
	// ##### SERVING_FOLDER #####
	char servingFolder[MAX_CONFIG_LINE_LEN];
	size_t servingFolderLen;

	// ##### PORT #####
	char port[MAX_CONFIG_PORT_LEN];

	// ##### CONNECTION #####
	size_t recvTimeout;

	// ##### LOGGING #####

	// ##### THREADS #####
	int maxConcurrentThreads;

	// ##### PIPELINE #####

	// ##### CACHE #####

	// ##### SECURITY #####

	// ##### SSL #####


} Config;

errno_t loadConfig(Config* config);


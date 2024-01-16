#pragma once

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define CONFIG_PATH "conf.txt"
#define MAX_CONFIG_LINE_LEN 256
#define MAX_CONFIG_PORT_LEN 6

typedef struct {
	// ##### SERVING_FOLDER #####
	char servingFolder[MAX_CONFIG_LINE_LEN];
	size_t servingFolderLen;

	// ##### PORT #####
	char port[MAX_CONFIG_PORT_LEN];
} Config;

errno_t loadConfig(Config* config);


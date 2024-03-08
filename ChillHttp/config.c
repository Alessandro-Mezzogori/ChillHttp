#include "config.h"

errno_t loadConfig(Config* config) {
	errno_t err = 0;
	FILE* file = NULL;

	err = fopen_s(&file, CONFIG_PATH, "r");
	if (err != 0 || file == NULL) {
		return err;
	}

	char buffer[MAX_CONFIG_LINE_LEN];
	while (fgets(buffer, sizeof(buffer), file)) {
		if(*buffer == '#' || isspace(*buffer)) {
			continue;
		}

		char* context = NULL;
		char* token = strtok_s(buffer, "=", &context);

		if(strcmp(token, "SERVING_FOLDER") == 0) {
			token = strtok_s(NULL, "\n", &context);
			strcpy_s(config->servingFolder, MAX_CONFIG_LINE_LEN, token);
			config->servingFolderLen = strlen(config->servingFolder);
		} else if(strcmp(token, "PORT") == 0) {
			token = strtok_s(NULL, "\n", &context);
			strcpy_s(config->port, MAX_CONFIG_PORT_LEN, token);
		} else if(strcmp(token, "MAX_CONCURRENT_THREADS") == 0) {
			token = strtok_s(NULL, "\n", &context);
			config->maxConcurrentThreads = strtol(token, NULL, 10);
		} else if(strcmp(token, "MAX_CONCURRENT_THREADS") == 0) {
			token = strtok_s(NULL, "\n", &context);
			config->maxConcurrentThreads = strtol(token, NULL, 10);
		} else if(strcmp(token, "CONNECTION:RECV_TIMEOUT") == 0) {
			token = strtok_s(NULL, "\n", &context);
			config->recvTimeout = strtol(token, NULL, 10) * 1000;
		}
	}
	fclose(file);

	return 0;
}

#include "router.h"

#define COALESCE_INT(a, b) a == 0 ? b : a

// TODO better return value ( error as parameter or return struct, file opening )
// for now NULL means not found, any error during the opening of a static file will be 
// assumed to be a 404 error
FILE* openServingFolderFile(const char* servingFolder, size_t servingFolderLen, const char* path) {
	const char trailingIndex[] = "/index.html";
	const size_t trailingIndexLen = sizeof(trailingIndex);

	// cases to handle:
	// - "/[page]" -> "/[page]/index.html"
	// - "/[page]/" -> "/[page]/index.html"
	// - "/[page].[ext]" -> "/[page].[ext]"

	size_t pathLen = strlen(path);

	BOOL hasTrailingSlash = path[pathLen - 1] == '/';
	BOOL hasExtension = strchr(path, '.') != NULL;
	size_t extensionLen = hasExtension == FALSE ? trailingIndexLen : 0;

	size_t filePathLength = 0;
	errno_t pathBuildingErr = 0;
	char* filePath = NULL;

	FILE* file = NULL;
	errno_t openFileErr = 0;

	if (hasTrailingSlash == TRUE) {
		pathLen -= 1;
	}

	filePathLength = servingFolderLen + pathLen + extensionLen;
	pathBuildingErr = 0;
	filePath = (char*) calloc(filePathLength + 1, sizeof(char));
	if(filePath == NULL || filePathLength <= 0) {
		goto _failure;
	}

	pathBuildingErr = memcpy_s(filePath, filePathLength*sizeof(char), servingFolder, servingFolderLen*sizeof(char));
	if(pathBuildingErr != 0) {
		LOG_ERROR("Error while building file path: servingFolder (memcpy_s error) %d", pathBuildingErr);
		goto _failure;
	}

	pathBuildingErr = memcpy_s(filePath + servingFolderLen, (filePathLength - servingFolderLen)*sizeof(char), path, pathLen*sizeof(char));
	if(pathBuildingErr != 0) {
		LOG_ERROR("Error while building file path: path (memcpy_s error) %d", pathBuildingErr);
		goto _failure;
	}

	if (extensionLen > 0) {
		pathBuildingErr = memcpy_s(filePath + servingFolderLen + pathLen, extensionLen*sizeof(char), trailingIndex, trailingIndexLen*sizeof(char));
		if(pathBuildingErr != 0) {
			LOG_ERROR("Error while building file path: trailing index (memcpy_s error) %d", pathBuildingErr);
			goto _failure;
		}
	}

	file = NULL;
	openFileErr = fopen_s(&file, filePath, "r");
	if (openFileErr != 0 || file == NULL) {
		LOG_ERROR("Error while opening file: %s (fopen_s error) %d", filePath, openFileErr);
		goto _failure;
	}

	return file;

_failure:
	free(filePath);
	return NULL;
}

errno_t readTxtFile(FILE* file, char** out, size_t chunkSize) {
	size_t _chunkSize = COALESCE_INT(chunkSize, 4096);

	size_t capacity = _chunkSize;
	size_t size = 0;
	char* buffer = (char*) malloc(capacity * sizeof(char));
	if(buffer == NULL) {
		return ENOMEM;
	}

	char* prevChunkEndPtr = buffer;
	while (TRUE)
	{
		size_t elementsRead = fread_s(prevChunkEndPtr, _chunkSize, sizeof(char), _chunkSize, file);
		prevChunkEndPtr = prevChunkEndPtr + elementsRead;
		size = prevChunkEndPtr - buffer;

		if (elementsRead < _chunkSize) {
			break;
		}

		if (size >= capacity) {
			capacity = capacity * 2;
			char* newBuffer = (char*) realloc(buffer, (capacity + 1) * sizeof(char));
			if (newBuffer == NULL) {
				free(buffer);
				return ENOMEM;
			}

			buffer = newBuffer;
			prevChunkEndPtr = buffer + size;
		}
	}

	buffer[size] = '\0';
	*out = buffer;

	return 0;
}

errno_t serveFile(const char* servingFolder, size_t servingFolderLen, HttpRequest* request, HttpResponse* response) {
	FILE* file = openServingFolderFile(servingFolder, servingFolderLen, request->path);
	char* fileContent = NULL;
	errno_t readErr = 0;

	if (file == NULL) {
		LOG_ERROR("Path %s not found", request->path);
		LOG_ERROR("File error {%s}", file);
		setHttpResponse(response, request->version, 404, NULL);
		goto _exit;
	}

	readErr = readTxtFile(file, &fileContent, 4096);
	fclose(file);
	if (readErr != 0) {
		setHttpResponse(response, request->version, 500, NULL);
		LOG_ERROR("Error while reading file: %s (readTxtFile error) %d", request->path, readErr);
		goto _exit_with_cleanup;
	}

	setHttpResponse(response, request->version, 200, fileContent);
	goto _exit;

_exit_with_cleanup:
	free(fileContent);
_exit:
	return 0;
}

errno_t registerRoute(const char* route, HTTP_METHOD method, RouteHandler routeHandler) {
	// TODO implement
	return 0;
}

// TODO better implementation, for now it just for a test
errno_t handleRequest(Config* config, HttpRequest* request, HttpResponse* response) {
	if (strcmp(request->path, "/test") == 0 && request->method == HTTP_POST) {
		return setHttpResponse(response, request->version, 203, strdup("test endpoint"));
	}
	else {
		return serveFile(config->servingFolder, config->servingFolderLen, request, response);
	}

	return 1;
}



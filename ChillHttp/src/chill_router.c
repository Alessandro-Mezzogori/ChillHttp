#include <chill_router.h>

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

	openFileErr = fopen_s(&file, filePath, "r");
	if (openFileErr != 0 || file == NULL) {
		LOG_ERROR("Error while opening file: %s (fopen_s error) %d", filePath, openFileErr);
		goto _failure;
	}

_failure:
	free(filePath);
	return file;
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
		response->statusCode = 404;
		goto _exit_with_error;
	}

	readErr = readTxtFile(file, &fileContent, 4096);
	fclose(file);
	if (readErr != 0) {
		response->statusCode = 500;
		LOG_ERROR("Error while reading file: %s (readTxtFile error) %d", request->path, readErr);
		goto _exit_with_cleanup;
	}

	setHttpResponse(response, request->version, 200, fileContent);
	goto _exit;

_exit_with_cleanup:
	free(fileContent);
_exit_with_error:
	return -1;
_exit:
	return 0;
}

errno_t serveError(const char* servingFolder, size_t servingFolderLen, HttpRequest* request, HttpResponse* response) {
	size_t filePathLength = servingFolderLen + 8 + 3 + 5 + 1;
	char* filePath = (char*) malloc((filePathLength + 1)*sizeof(char));

	if (filePath == NULL) {
		LOG_ERROR("Error while allocating memory for file path: %d", ENOMEM);
		goto _failure;
	}

	sprintf_s(filePath, filePathLength, "%s\\errors\\%hu.html", servingFolder, response->statusCode);

	char* fileContent = NULL;
	errno_t readErr = 0;
	FILE* file = NULL;
	errno_t openFileErr = fopen_s(&file, filePath, "r");
	if (openFileErr != 0 || file == NULL) {
		LOG_ERROR("Error while opening file: %s (fopen_s error) %d", filePath, openFileErr);
		goto _failure;
	}

	readErr = readTxtFile(file, &fileContent, 4096);
	fclose(file);
	if (readErr != 0) {
		goto _exit_with_cleanup;
	}

	unsigned short statusCode = response->statusCode == 0 ? 500 : response->statusCode;
	setHttpResponse(response, request->version, statusCode, fileContent);

	free(filePath);
	goto _exit;

_exit_with_cleanup:
	free(fileContent);
_failure:
	if (filePath != NULL){
		free(filePath);
	}

	if (response->statusCode != 500) {
		response->statusCode = 500;
		return serveError(servingFolder, servingFolderLen, request, response);
	}
	else {
		setHttpResponse(response, request->version, 500, strdup("500 - Internal Server Error"));
	}
_exit:
	return 0;
}

errno_t registerRoute(Route* route, const char* path, HTTP_METHOD method, RouteHandler routeHandler) {
	if(route == NULL || strlen(path) < 1) {
		return EINVAL;
	}

	route->route = path;
	route->catchAll = path[0] == '*';
	route->method = method;
	route->routeHandler = routeHandler;

	return 0;
}

int compareRoute(void* ctx, const void* a, const void* b) {
	Route* routeA = (Route*)a;
	Route* routeB = (Route*)b;

	if(!routeA && !routeB) return 0;
	if(!routeA) return 1;
	if(!routeB) return -1;

	if(routeA->catchAll == true && routeB->catchAll == false) {
		return 1;
	}

	if(routeA->catchAll == false && routeB->catchAll == true) {
		return -1;
	}

	return 0;
}

errno_t handleRequest(Route* routes, size_t routesSize, const Config* config, HttpRequest* request, HttpResponse* response) {
	errno_t err = 0;

	qsort_s(routes, routesSize, sizeof(Route), compareRoute, NULL);
	for(size_t i = 0; i < routesSize; i++) {
		Route route = routes[i];

		if(route.method == request->method && (route.catchAll == true || strcmp(route.route, request->path) == 0)) {
			err = route.routeHandler(config, request, response);
			break;
		}
	}

	if (err != 0) {
		err = serveError(config->servingFolder, config->servingFolderLen, request, response);
	}

	return err;
}

/// <summary>
/// Handles error responses
/// </summary>
/// <param name="config"></param>
/// <param name="request"></param>
/// <param name="response"></param>
/// <returns>if everything ok 0, 1 if the statuscode is not an error</returns>
errno_t handleError(const Config* config, HttpRequest* request, HttpResponse* response) {
	if(response == NULL) {
		return EINVAL;
	}

	if(response->statusCode < 400 || response->statusCode > 599) {
		return 1;
	}

	return serveError(config->servingFolder, config->servingFolderLen, request, response);
}



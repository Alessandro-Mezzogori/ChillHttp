#include "thread.h"

#define COALESCE_INT(a, b) a == 0 ? b : a

// TODO better return value ( error as parameter or return struct, file opening )
// for now NULL means not found, any error during the opening of a static file will be 
// assumed to be a 404 error
FILE* openServingFolderFile(char* servingFolder, size_t servingFolderLen, const char* requestPath) {
	const char trailingIndex[] = "/index.html";
	const size_t trailingIndexLen = 11;

	// cases to handle:
	// - "/[page]" -> "/[page]/index.html"
	// - "/[page]/" -> "/[page]/index.html"
	// - "/[page].[ext]" -> "/[page].[ext]"

	const char* path = requestPath;
	size_t pathLen = strlen(path);

	BOOL hasTrailingSlash = path[pathLen - 1] == '/';
	BOOL hasExtension = strchr(path, '.') != NULL;
	size_t extensionLen = hasExtension == FALSE ? trailingIndexLen : 0;

	if (hasTrailingSlash == TRUE) {
		pathLen -= 1;
	}

	size_t filePathLength = servingFolderLen + pathLen + extensionLen;
	errno_t pathBuildingErr = 0;
	char* filePath = (char*) calloc(filePathLength + 1, sizeof(char));
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

	FILE* file = NULL;
	errno_t* openFileErr = fopen_s(&file, filePath, "r");
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

errno_t buildResponse(HashTable headers, char* body) {
	return 0;
}

DWORD threadFunction(void* lpParam) {
	PSOCKTD pdata = (PSOCKTD)lpParam;
	PMTData mtData = pdata->pmtData;
	int threadId = pdata->threadId;
	SOCKET socket = pdata->socket;

	LOG_INFO("Thread {%d} socket: {%d} initiliazed", threadId, socket);

	size_t servingFolderLength = strlen(mtData->servingFolder);

	int connectionResult = 0;
	char buffer[8192]; // TODO dynamic recv buffer that can read chunked http data
	memset(buffer, 0, sizeof(buffer));
	do {
		// TODO handle chunked data
		// specification https://stackoverflow.com/questions/49821687/how-to-determine-if-i-received-entire-message-from-recv-calls
		// TLDR
		// - read heading ( till \r\n )
		// - read headers ( till \r\n\r\n )
		// - analyze headers to determine if response body is present
		// - read body based on the 5 ways 
		connectionResult = recv(socket, buffer, sizeof(buffer), 0);
		if (connectionResult > 0) {
			LOG_INFO("Socket {%d} bytes received: %d", socket, connectionResult);
			LOG_INFO("Socket {%d}\r\nMessage: %s", socket, &buffer);

			HttpRequest* request = parseHttpRequest(buffer);

			HashEntry* connection = hashtableLookup(request->headers, "Connection");
			if(connection != NULL && strcmp(connection->value, "close") == 0) {
				connectionResult = 0;
				LOG_INFO("Socket {%d} closing connection", socket);
			}

			// read path
			FILE* file = openServingFolderFile(mtData->servingFolder, servingFolderLength, request->path);
			if(file == NULL) {
				LOG_ERROR("Path %s not found", request->path);
				LOG_ERROR("File error {%s}", file);

				// TODO cleanup e shutdown request with error 404
				freeHttpRequest(request);
				return 1;
			}

			char* fileContent = NULL;
			errno_t readErr = readTxtFile(file, &fileContent, 4096);
			fclose(file);

			if (readErr != 0) {
				LOG_ERROR("Error while reading file: %s (readTxtFile error) %d", request->path, readErr);
				freeHttpRequest(request);
				return 1;
			}

			// takes ownership of fileContent
			HttpResponse* response = createHttpResponse(request->version, 200, NULL, fileContent);
			if(response == NULL) {
				LOG_ERROR("Error while creating response");
				freeHttpRequest(request);
				free(fileContent);
				return 1;
			}

			size_t responseBufferLen = 0;
			char* responseBuffer;
			errno_t buildErr = buildHttpResponse(response, &responseBuffer, &responseBufferLen);
			if(buildErr != 0) {
				LOG_ERROR("Error while building response: %d", buildErr);
				freeHttpRequest(request);
				freeHttpResponse(response);
				return 1;
			}

			int sendResult = send(socket, responseBuffer, responseBufferLen, 0);


			if(sendResult == SOCKET_ERROR) {
				LOG_FATAL("Socket {%d} send failed: %d", socket, WSAGetLastError());
				closesocket(socket);

				free(responseBuffer);
				freeHttpRequest(request);
				freeHttpResponse(response);
				WSACleanup();
				return 1;
			}

			LOG_INFO("Socket {%d} bytes sent: %d", socket, sendResult);
			LOG_INFO("Socket:{%d} Message:\r\n%s", socket, responseBuffer);

			free(responseBuffer);
			freeHttpRequest(request);
			freeHttpResponse(response);
		}
		else if (connectionResult == 0) {
			LOG_INFO("Socket {%d} connection closing...", socket);
		}
		else {
			LOG_FATAL("Socket {%d} cecv failed: %d", socket, WSAGetLastError());
			closesocket(socket);
			WSACleanup();
			return 1;
		}
	} while (connectionResult > 0);

	if (shutdown(socket, SD_SEND) == SOCKET_ERROR) {
		LOG_FATAL("Socket {%d} Shutdown failed: %d", socket, WSAGetLastError());
		closesocket(socket);
		WSACleanup();
		return 1;
	}

	LOG_TRACE("Closing thread {%d} socket {%d}", threadId, socket);
	closesocket(socket);

	pdata->isActive = FALSE;
	return 0;
}

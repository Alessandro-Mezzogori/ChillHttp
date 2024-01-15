#include "http.h"

#define HTTP_VERSION_STRING_LEN 8

HTTP_METHOD parseHttpMethod(char* method) {
	if (strcmp(method, "GET") == 0) {
		return HTTP_GET;
	}
	else if (strcmp(method, "HEAD") == 0) {
		return HTTP_HEAD;
	}
	else if (strcmp(method, "POST") == 0) {
		return HTTP_POST;
	}
	else if (strcmp(method, "PUT") == 0) {
		return HTTP_PUT;
	}
	else if (strcmp(method, "DELETE") == 0) {
		return HTTP_DELETE;
	}
	else if (strcmp(method, "CONNECT") == 0) {
		return HTTP_CONNECT;
	}
	else if (strcmp(method, "OPTIONS") == 0) {
		return HTTP_OPTIONS;
	}
	else if (strcmp(method, "TRACE") == 0) {
		return HTTP_TRACE;
	}
	else if (strcmp(method, "PATCH") == 0) {
		return HTTP_PATCH;
	}

	return HTTP_UNKNOWN;
}

HTTP_VERSION parseHttpVersion(char* version) {
	if (strcmp(version, "HTTP/1.0") == 0) {
		return HTTP_1_0;
	}
	else if (strcmp(version, "HTTP/1.1") == 0) {
		return HTTP_1_1;
	}
	else if (strcmp(version, "HTTP/2.0") == 0) {
		return HTTP_2_0;
	}

	return HTTP_UNKNOWN_VERSION;
}

int sanitizeHttpRequest(HttpRequest* request) {
	char* foundFolderBack = strstr(request->path, "..");
	if (foundFolderBack) {
		free(request->path);
		request->path = calloc(2, sizeof(char));
		if(request->path == NULL) {
			return -1;
		}

		strcpy_s(request->path, 2*sizeof(char), "/");
	};

	return 0;
}

void freeHttpRequest(HttpRequest* request) {
	free(request->path);
	hashtableFree(request->headers);
	free(request->body);
}

HttpResponse* createHttpResponse(HTTP_VERSION version, short statusCode, HashTable* headers, char* body) {
	HttpResponse* response = (HttpResponse*) malloc(sizeof(HttpResponse));
	if(response == NULL) {
		return NULL;
	}

	if (headers == NULL) {
		headers = hashtableCreate();

		if (headers == NULL) {
			free(response);
			return NULL;
		}
	}

	response->version = version;
	response->statusCode = statusCode;
	response->headers = headers;
	response->body = body;

	return response;
}

void freeHttpResponse(HttpResponse* response) {
	hashtableFree(response->headers);
	free(response->body);
	free(response);
}

/* Build response helpers */
errno_t buildHttpResponse(HttpResponse* response, char** buffer, size_t* bufferSize) {
	// ##### Resposne size computation #####

	// LEN = 8 (HTTP VER) + 1 + 3 (STATUSCODE) + 1 + X (STATUSMSG) + 2 (CRLF)
	// LEN = [X (HEADER) + 2 (CRLF) ] * headers->size
	// LEN = 2 (CRLF)	
	// LEN = X (BODY)
	size_t statusMsgLen = 0;
	size_t headingLen = 8 + 1 + 3 + 1 + statusMsgLen + 2;

	const size_t headerBodySeparatorLen = 2; // CRLF
	size_t bodyLen = strlen(response->body);

	size_t headersLen = 0;
	bool hasContentLength = false;
	HashTable* headers = response->headers;
	if (headers == NULL) {
		LOG_ERROR("Headers are NULL");
		return 1;
	}

	// Content-Length
	char contentLengthString[32];
	sprintf_s(contentLengthString, sizeof(contentLengthString), "%zu", bodyLen);
	// TODO maybe for the current implementation it is better to lookup and create only if not found
	hashtableAdd(headers, "Content-Length", contentLengthString);

	// TODO optimize: hashtable can store the byte size that is occupying, this leads to one less non-adjacent 
	// memory access at the cost of 8 bytes (size_t, probabily overkill, more for consistency)
	for (size_t i = 0; i < headers->hashsize; i++) {
		HashEntry* entry = headers->entries[i];

		while (entry != NULL) {
			// Header: value\r\n
			headersLen += strlen(entry->name) + 2 + strlen(entry->value) + 2;
			entry = entry->next;
		}
	}
	size_t totalLen = headingLen + headersLen + headerBodySeparatorLen + bodyLen;

	// ##### Response building #####
	*buffer = (char*) malloc((totalLen + 1)*sizeof(char));
	*bufferSize = totalLen;
	if(*buffer == NULL) {
		return ENOMEM;
	}

	char* bufferPtr = *buffer;
	size_t bufferRemainingLen = totalLen;

	static const char* httpVersionStrings[] = {
		"",
		"HTTP/1.0",
		"HTTP/1.1",
		"HTTP/2.0"
	};

	// HTTP HEADING

	// SPACE and STATUS CODE
	char statusCodeString[HTTP_VERSION_STRING_LEN + 1 + 3 + 3 + 1];
	sprintf_s(statusCodeString, sizeof(statusCodeString), "%s %hu \r\n", httpVersionStrings[response->version], response->statusCode); // TODO error handleing
	size_t statusCodeStringLen = strlen(statusCodeString);
	memcpy_s(bufferPtr, bufferRemainingLen, statusCodeString, statusCodeStringLen);
	bufferPtr += statusCodeStringLen;
	bufferRemainingLen -= statusCodeStringLen;
		
	// HEADERS
	// TODO check if headers go above 8192 => return error 500 in case
	if (headers != NULL) {
		char header[8192];
		for (size_t i = 0; i < headers->hashsize; i++) {
			HashEntry* entry = headers->entries[i];

			while (entry != NULL) {
				// TODO error handling
				sprintf_s(header, sizeof(header), "%s: %s\r\n", entry->name, entry->value);
				size_t headerLen = strlen(header);

				// TODO error handling
				memcpy_s(bufferPtr, bufferRemainingLen, header, headerLen);
				bufferPtr += headerLen;
				bufferRemainingLen -= headerLen;

				entry = entry->next;
			}
		}
	}

	// HEADER BODY SEPARATOR
	memcpy_s(bufferPtr, bufferRemainingLen, "\r\n", 2);
	bufferPtr += 2;
	bufferRemainingLen -= 2;

	// BODY
	memcpy_s(bufferPtr, bufferRemainingLen, response->body, bodyLen);
	bufferPtr += bodyLen;
	bufferRemainingLen -= bodyLen;

	bufferPtr[0] = '\0';

	LOG_TRACE("Response built len: %d", bufferPtr - *buffer);
	LOG_TRACE("Response remaining allocated len: %d", bufferRemainingLen);
	if(bufferRemainingLen != 0) {
		LOG_WARN("Response remaining allocated len is not 0");
	}

	return 0;
}

enum RecvRequestState {
	RECV_Start,
	RECV_Heading,
	RECV_Headers,
	RECV_BodyStrategy,
	RECV_Body,
};

size_t findWordBounds(char** start) {
	while (isspace(**start)) {
		(*start)++;
	}

	char* end = *start;
	while(!isspace(*end)) {
		end++;
	}

	return end - *start;
}

errno_t recvRequest(SOCKET socket, HttpRequest* req) {
	// ##### Request init globals #####
	errno_t err = 0;

	// ##### Request parsing pipeline setup #####
	enum RecvRequestState state = RECV_Start;	

	const size_t bufferchunk = 4096; //32;
	size_t size = 0;
	size_t capacity = bufferchunk * 4;
	char* buffer = (char*) malloc(capacity * sizeof(char));
	if (req == NULL) {
		err = ENOMEM;
	}

	req->method = HTTP_UNKNOWN;
	req->path = NULL;
	req->version = HTTP_UNKNOWN_VERSION;
	req->headers = hashtableCreate();
	req->body = NULL;

	if(req->headers == NULL) {
		err = ENOMEM;
		goto _cleanup_request;
	}

	// ##### Request parsing pipeline #####

	bool stopRecv = false;
	int recvResult = 0;
	size_t bytesRead = 0;
	long bodyLen = 0;
	do {
		recvResult = recv(socket, buffer + size, bufferchunk, 0);
		if (recvResult < 0) {
			LOG_ERROR("recv failed: %d", WSAGetLastError());
			err = WSAGetLastError();
			goto _cleanup_request;
		}

		size += recvResult;
		if (size >= capacity) {
			capacity = capacity * 2;
			char* newBuffer = (char*) realloc(buffer, capacity * sizeof(char));
			if(newBuffer == NULL) {
				err = ENOMEM;
				goto _cleanup_request;
			}

			buffer = newBuffer;
		}

		stopRecv = recvResult == 0;
		if (state == RECV_Start) {
			char* headingEndPtr = strchr(buffer + bytesRead, '\n');
			char* headingPtr = buffer + bytesRead;
			if (headingEndPtr != NULL) {
				size_t methodLength = findWordBounds(&headingPtr);
				headingPtr[methodLength] = '\0';
				req->method = parseHttpMethod(headingPtr);
				headingPtr += methodLength + 1;

				size_t pathLength = findWordBounds(&headingPtr);
				headingPtr[pathLength] = '\0';
				req->path = strdup(headingPtr);
				headingPtr += pathLength + 1;

				size_t versionLength = findWordBounds(&headingPtr);
				headingPtr[versionLength] = '\0';
				req->version = parseHttpVersion(headingPtr);
				headingPtr += versionLength + 1;

				state = RECV_Headers;
				bytesRead = headingEndPtr - buffer + 1;
			}
		}

		if (state == RECV_Headers) {
			while (true) {
				char* headingPtr = buffer + bytesRead;
				if (memcmp(headingPtr, "\r\n", 2) == 0 || memcmp(headingPtr, "\n", 1) == 0) {
					state = RECV_BodyStrategy;
					bytesRead += (*headingPtr == '\n' ? 1 : 2);
					break;
				}

				char* headingEndPtr = strchr(headingPtr, '\n');
				if (headingEndPtr == NULL) {
					break;
				}

				char* separatorPtr = strchr(headingPtr, ':');
				if (separatorPtr == NULL) {
					break;
				}

				*separatorPtr = '\0';
				separatorPtr += 1;
				while (isspace(*separatorPtr)) {
					separatorPtr++;
				}

				char* end = headingEndPtr;
				while (isspace(*end)) {
					end--;
				}
				end[1] = '\0';

				hashtableAdd(req->headers, headingPtr, separatorPtr);
				bytesRead = headingEndPtr - buffer + 1;
			}
		}

		if (state == RECV_BodyStrategy) {
			LOG_DEBUG("Parased headers:");
			hashtablePrint(LOG_FP, req->headers);

			HashEntry* contentLength = hashtableLookup(req->headers, "Content-Length");
			if(contentLength != NULL) {
				bodyLen = strtol(contentLength->value, NULL, 10);
				state = RECV_Body;
			}
			else {
				LOG_ERROR("Content-Length header not found");
				LOG_ERROR("No other end body strategy supported");
				err = 1; // TODO trovare un errore corretto
				goto _cleanup_request;
			}
		}

		if (state == RECV_Body) {
			if (size - bytesRead >= bodyLen) {
				char* body = buffer + bytesRead;
				req->body = malloc((bodyLen + 1)*sizeof(char));
				req->body[bodyLen] = '\0';
				memcpy_s(req->body, bodyLen, body, bodyLen);

				stopRecv = true;
			}
		}
	} while (!stopRecv);

	// TODO errror handling
	int sanitizeResult = sanitizeHttpRequest(req);
	if(sanitizeResult != 0) {
		err = 1;
		goto _cleanup_request;
	}

	return 0;
	// ##### Request error cleanup #####
_cleanup_request:
	free(buffer);
	freeHttpRequest(req);
	//free(request);
	return err;
}

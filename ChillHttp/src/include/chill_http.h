#pragma once

#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <winsock2.h>

#include <chill_hashtable.h>
#include <chill_log.h>

typedef enum HTTP_METHOD {
	HTTP_UNKNOWN = 0,
	HTTP_GET = 1,
	HTTP_HEAD = 2,
	HTTP_POST = 3,
	HTTP_PUT = 4,
	HTTP_DELETE = 5,
	HTTP_CONNECT = 6,
	HTTP_OPTIONS = 7,
	HTTP_TRACE = 8,
	HTTP_PATCH = 9
} HTTP_METHOD;

typedef enum HTTP_VERSION {
	HTTP_UNKNOWN_VERSION = 0,
	HTTP_1_0 = 1,
	HTTP_1_1 = 2,
	HTTP_2_0 = 3
} HTTP_VERSION;

typedef struct HttpRequest {
	enum HTTP_METHOD method;
	char* path; 
	enum HTTP_VERSION version;
	HashTable* headers;
	char* body;
} HttpRequest;

typedef struct HttpResponse {
	enum HTTP_VERSION version;
	unsigned short statusCode;
	HashTable* headers;
	char* body;
	size_t bodySize;
} HttpResponse;

errno_t recvRequest(SOCKET socket, HttpRequest* req);
void freeHttpRequest(HttpRequest* request);

errno_t createHttpResponse(HttpResponse* response);
errno_t setHttpResponse(HttpResponse* response, HTTP_VERSION version, short statusCode, const char* body);
errno_t buildHttpResponse(HttpResponse* response, char** buffer, size_t* bufferSize);
void freeHttpResponse(HttpResponse* response);

errno_t setHttpResponseBody(HttpResponse* response, const char* body);


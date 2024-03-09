#pragma once

#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <winsock2.h>

#include <chill_hashtable.h>
#include <chill_log.h>

typedef enum HTTP_METHOD {
	HTTP_UNKNOWN,
	HTTP_GET,
	HTTP_HEAD,
	HTTP_POST,
	HTTP_PUT,
	HTTP_DELETE,
	HTTP_CONNECT,
	HTTP_OPTIONS,
	HTTP_TRACE,
	HTTP_PATCH
} HTTP_METHOD;

typedef enum HTTP_VERSION {
	HTTP_UNKNOWN_VERSION,
	HTTP_1_0,
	HTTP_1_1,
	HTTP_2_0
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
} HttpResponse;

errno_t recvRequest(SOCKET socket, HttpRequest* req);
void freeHttpRequest(HttpRequest* request);

errno_t createHttpResponse(HttpResponse* response);
errno_t setHttpResponse(HttpResponse* response, HTTP_VERSION version, short statusCode, char* body);
errno_t buildHttpResponse(HttpResponse* response, char** buffer, size_t* bufferSize);
void freeHttpResponse(HttpResponse* response);


#pragma once

#include "hashtable.h"
#include<stdlib.h>
#include<ctype.h>

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

HttpRequest* parseHttpRequest(const char* request);
void freeHttpRequest(HttpRequest* request);
void sanitizeHttpRequest(HttpRequest* request);

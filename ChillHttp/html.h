#pragma once
#include<stdlib.h>

typedef struct HttpRequest {
	char* method;
	char* path;
	char* version;
	char* headers;
	char* body;
} HttpRequest;

HttpRequest parseHttpRequest(const char* request) {
	struct HttpRequest httpRequest;
	httpRequest.method = NULL;
	httpRequest.path = NULL;
	httpRequest.version = NULL;
	httpRequest.headers = NULL;
	httpRequest.body = NULL;

	return httpRequest;
}

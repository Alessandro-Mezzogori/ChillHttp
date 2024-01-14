#include "http.h"

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

HttpRequest* parseHttpRequest(const char* request) {
	HttpRequest* req = (HttpRequest*) malloc(sizeof(HttpRequest) * 1);
	if(req == NULL) {
		return NULL;
	}

	req->method = HTTP_GET;
	req->version = HTTP_1_1;
	req->headers = NULL;
	req->body = NULL;

	char* requestCopy = strdup(request);
	if (requestCopy == NULL) {
		freeHttpRequest(req);
		return NULL;
	}

	char* bodyStart = strstr(requestCopy, "\r\n\r\n");
	if(bodyStart != NULL) {
		bodyStart[0] = '\0';
		bodyStart += 4;
	}

	char* requestCopyNextToken = NULL;
	char* line = strtok_s(requestCopy, "\r\n", &requestCopyNextToken);

	char* lineNextToken = NULL;
	char* method = strtok_s(line, " ", &lineNextToken);

	req->method = parseHttpMethod(method);

	char* path = strtok_s(NULL, " ", &lineNextToken);
	req->path = strdup(path); 

	char* version = strtok_s(NULL, " ", &lineNextToken);
	req->version = parseHttpVersion(version);

	char* header = strtok_s(NULL, "\r\n", &requestCopyNextToken);
	if(header != NULL) {
		req->headers = hashtableCreate();
	}

	while (header != NULL) {
		char* delimiter = strchr(header, ':');
		*delimiter = '\0';
		delimiter++;

		hashtableAdd(req->headers, header, delimiter);
		header = strtok_s(NULL, "\r\n", &requestCopyNextToken);
	};

	if (bodyStart != NULL) {
		// TODO capire se 3 o 2
		req->body = strdup(bodyStart);
	}

	free(requestCopy);
	return req;
}

void freeHttpRequest(HttpRequest* request) {
	free(request->path);
	hashtableFree(request->headers);
	free(request->body);
	free(request);
}
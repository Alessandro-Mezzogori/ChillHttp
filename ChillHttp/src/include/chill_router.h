#pragma once

#include <string.h>

#include <chill_http.h>
#include <chill_log.h>
#include <chill_config.h>
#include <chill_errors.h>


typedef errno_t (RouteHandler)(const Config* config, HttpRequest* request, HttpResponse* response);

typedef struct Route {
	const char* route;
	HTTP_METHOD method;
	RouteHandler* routeHandler;
	bool catchAll;
} Route;

errno_t registerRoute(Route* route, const char* path, HTTP_METHOD method, RouteHandler routeHandler);
errno_t handleRequest(Route* routes, size_t routesSize, const Config* config, HttpRequest* request, HttpResponse* response);
errno_t handleError(const Config* config, HttpRequest* request, HttpResponse* response);
errno_t serveFile(const char* servingFolder, size_t servingFolderLen, HttpRequest* request, HttpResponse* response);

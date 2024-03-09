#pragma once

#include "http.h"
#include "log/log.h"
#include "config.h"
#include "chillerrors.h"


typedef HttpResponse* (RouteHandler)(HttpRequest* request);;

errno_t registerRoute(const char* route, HTTP_METHOD method, RouteHandler routeHandler);
errno_t handleRequest(Config* config, HttpRequest* request, HttpResponse* response);
errno_t handleError(Config* config, HttpRequest* request, HttpResponse* response);

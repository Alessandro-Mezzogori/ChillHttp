#pragma once

#include <chill_http.h>
#include <chill_connection.h>
#include <chill_config.h>
#include <chill_router.h>
#include <chill_lua.h>

#define PIPELINE_STEP_META "chill.pipeline"
#define PIPELINE_CONTEXT_META "chill.pipeline.context"
#define PIPELINE_STEP_ARGS_META "chill.pipeline.stepargs"

#define PIPELINE_ROUTES_META "chill.routes"
#define PIPELINE_ROUTES_ROUTE_META "chill.routes.route"
#define PIPELINE_CONFIG_META "chill.config"
#define PIPELINE_HASHTABLE_META "chill.hashtable"

#define PIPELINE_HTTPREQUEST_META "chill.http.request"
#define HTTPREQUEST_HEADER_INDEX 1
#define HTTPRERESPONSE_HEADER_INDEX 1

#define PIPELINE_HTTPRESPONSE_META "chill.http.response"
#define PIPELINE_CONNECTIONDATA_META "chill.connection"

#define PIPELINE_CONTEXT_REQUEST_INDEX 1
#define PIPELINE_CONTEXT_RESPONSE_INDEX 2
#define PIPELINE_CONTEXT_CONNECTION_INDEX 3

typedef struct HashTableWrapper HashTableWrapper;
typedef struct HttpRequestWrapper HttpRequestWrapper;
typedef struct HttpResponseWrapper HttpResponseWrapper;
typedef struct RoutesWrapper RoutesWrapper;
typedef struct RouteWrapper RouteWrapper;
typedef struct ConfigWrapper ConfigWrapper;
typedef struct ConnectionDataWrapper ConnectionDataWrapper;
typedef struct PipelineLuaStep PipelineLuaStep;
typedef struct ContextWrapper ContextWrapper;

int lua_openlibs_pipeline(lua_State* L);

struct HashTableWrapper {
	HashTable* ht;
	bool isReadOnly;
};

struct HttpRequestWrapper {
	HttpRequest* request;
};

struct HttpResponseWrapper {
	HttpResponse* response;
};

struct RoutesWrapper {
	Route* routes;
	size_t size;
};

struct RouteWrapper {
	Route* route;
};

struct ConfigWrapper {
	Config* config;
};

struct ConnectionDataWrapper {
	cSocket* connectionData;
};


struct PipelineLuaStep {
	char id[255];
	int handlerRef;
};

struct ContextWrapper {
	// TODO: make these const
	HttpRequest* request;
	HttpResponse* response;
	cSocket* connectionData;

	Config* config;

	size_t stepsSize;
	size_t currentStep;

	Route* routes;
	size_t routesSize;

	struct PipelineLuaStep steps[1];
};


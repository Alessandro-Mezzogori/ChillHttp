#pragma once

#include <chill_http.h>
#include <chill_connection.h>
#include <chill_config.h>
#include <chill_router.h>
#include <chill_lua.h>
#include <chill_pipeline_lualibs.h>

#define PIPELINE_SCRIPT "pipeline.lua"

typedef struct PipelineContextInit PipelineContextInit;

struct PipelineContextInit {
	HttpRequest* request;
	HttpResponse* response;
	ConnectionData* connectionData;
	Config* const config;

	Route* routes;
	size_t routesSize;
};

// errno_t next(ContextWrapper* context);
errno_t runPipeline(PipelineContextInit* init);

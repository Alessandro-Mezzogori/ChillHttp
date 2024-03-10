#pragma once
#include <chill_http.h>
#include <chill_connection.h>
#include <chill_config.h>
#include <chill_router.h>
#include <chill_lua.h>

#define PIPELINE_SCRIPT "pipeline.lua"

typedef struct PipelineContextInit PipelineContextInit;
typedef struct PipelineContext PipelineContext;
typedef struct PipelineStep PipelineStep;

struct PipelineContextInit {
	HttpRequest* request;
	HttpResponse* response;
	ConnectionData* connectionData;
	Config* const config;

	Route* routes;
	size_t routesSize;
};

struct PipelineStep {
	char id[255];
	int handlerRef;
	// const errno_t (*function)(PipelineContext*);
	// const void* (*cleanup)(void);
	// const PipelineStep* next;
};

struct PipelineContext {
	//const HttpRequest* request;
	//const HttpResponse* response;
	//const ConnectionData* connectionData;

	// TODO: make these const
	HttpRequest* request;
	HttpResponse* response;
	ConnectionData* connectionData;
	Config* config;

	size_t stepsSize;
	size_t currentStep;

	Route* routes;
	size_t routesSize;

	struct PipelineStep steps[1];
};

// errno_t next(PipelineContext* context);
errno_t startPipeline(PipelineContextInit* init);
errno_t closePipeline(PipelineContext* context);

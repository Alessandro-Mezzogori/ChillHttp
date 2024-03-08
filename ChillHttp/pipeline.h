#pragma once
#include "http.h"
#include "connection.h"
#include "config.h"

typedef struct PipelineContext PipelineContext;
typedef struct PipelineStep PipelineStep;

struct PipelineContext {
	//const HttpRequest* request;
	//const HttpResponse* response;
	//const ConnectionData* connectionData;

	HttpRequest* request;
	HttpResponse* response;
	ConnectionData* connectionData;
	Config* const config;
	PipelineStep* currentStep;
} ;

struct PipelineStep {
	const char* name;
	const errno_t* (*function)(PipelineContext*);
	const void* (*cleanup)(void);
	const PipelineStep* next;
};

errno_t next(PipelineContext* context);
errno_t startPipeline(PipelineContext* context);

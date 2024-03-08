#pragma once
#include "http.h"

typedef struct {
	HttpRequest* request;
	HttpResponse* response;
} PipelineContext;

typedef struct {
	char* name;
	errno_t* (*function)(PipelineContext*);
	void* (*cleanup)(void);
} PipelineStep;

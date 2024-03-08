#include "pipeline.h"

errno_t next(PipelineContext* context) {
	if (context->currentStep == NULL) {
		return 0;
	}

	context->currentStep = context->currentStep->next;
	if(context->currentStep == NULL) {
		return 0;
	}

	return context->currentStep->function(context);
}

errno_t startPipeline(PipelineContext* context) {
	if (context->currentStep == NULL) {
		return 0;
	}

	return context->currentStep->function(context);
}
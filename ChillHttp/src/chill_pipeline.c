#include <chill_pipeline.h>

#define PIPELINE_STEP_META "chill.pipeline"
#define PIPELINE_CONTEXT_META "chill.pipeline.context"
#define PIPELINE_STEP_ARGS_META "chill.pipeline.stepargs"

errno_t next(PipelineContext* context) {
	return 0;
}

errno_t request2lua(lua_State* L, HttpRequest* request) {
	lua_newtable(L);
	lua_pushinteger(L, request->method);
	lua_setfield(L, -2, "method");

	lua_pushstring(L, request->path);
	lua_setfield(L, -2, "path");

	lua_pushstring(L, request->body);
	lua_setfield(L, -2, "body");

	lua_newtable(L);

	HashTable* ht = request->headers;
	for (int i = 0; i < HASHSIZE; i++) {
		HashEntry* entry = ht->entries[i];
		while (entry != NULL) {
			lua_pushstring(L, entry->value);
			lua_setfield(L, -2, entry->name);

			entry = entry->next;
		}
	}

	stackDump(L);
	lua_setfield(L, -2, "headers");
}

errno_t startPipeline(PipelineContextInit* init) {
	// setup context
	// call first step

	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	luaopen_pipeline(L);
	// luaopen_array(L);

	int r = luaL_dofile(L, PIPELINE_SCRIPT);
	if (r != LUA_OK) {
		LOG_ERROR("Error: %s", lua_tostring(L, -1));
		lua_close(L);
		return -1;
	}

	lua_len(L, -1);
	int tableLen = lua_tointeger(L, -1);
	LOG_TRACE("Pipeline table length: %d", tableLen);
	lua_pop(L, 1);

	size_t nbytes = sizeof(PipelineContext) + tableLen*sizeof(PipelineStep);
	PipelineContext* a = (PipelineContext*) lua_newuserdata(L, nbytes);
	a->request = init->request;
	a->response = init->response;
	a->connectionData = init->connectionData;
	a->config = init->config;
	a->routes = init->routes;
	a->routesSize = init->routesSize;
	a->stepsSize = tableLen;
	a->currentStep = 0;

	for(int i = 0; i < tableLen; i++) {
		PipelineStep* step = &a->steps[i];
		step->handlerRef = LUA_NOREF;
		step->id[0] = 0;
	}

	luaL_getmetatable(L, PIPELINE_STEP_ARGS_META);
	lua_setmetatable(L, -2);

	lua_insert(L, -2);	// move usedata before function)
	setupPipeline(L, a);
	lua_insert(L, -2);	// move usedata before function)

	if(a->stepsSize == 0) {
		LOG_ERROR("No steps in pipeline");
		closePipeline(L, a);
		return -1;
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, a->steps[0].handlerRef);	// push function
	lua_insert(L, -2);	// move usedata before function)

	if (lua_isfunction(L, -2)) {
		int err = lua_pcall(L, 1, 0, 0);
		if (err != 0) {
			LOG_ERROR("error running function: %s", lua_tostring(L, -1));
		}
	}

	closePipeline(L, a);
	return 0;
}

errno_t getStep(lua_State* L, PipelineStep* step) {
	int fieldType = lua_getfield(L, -1, "id");
	if (fieldType != LUA_TSTRING ) {
		LOG_ERROR("Step.id not a string or not found");
		luaL_error(L, "Step.id not a string or not found");
		return -1;
	}

	const char* id = luaL_checkstring(L, -1);
	if (strlen(id) > 255) {
		LOG_ERROR("Step id too long: %s", id);
		luaL_error(L, "Step id too long (max 255): %s", id);
		return -1;
	}
	strcpy(step->id, id);
	lua_pop(L, 1);

	fieldType = lua_getfield(L, -1, "handler");
	if(fieldType != LUA_TFUNCTION) {
		LOG_ERROR("Step.handler not a function or not found");
		luaL_error(L, "Step.handler not a function or not found");
		return -1;
	}
	step->handlerRef = luaL_ref(L, LUA_REGISTRYINDEX);

	return 0;
}

errno_t setupPipeline(lua_State*L, PipelineContext* context) {
	errno_t err = 0;

	LOG_TRACE("##### Pipeline building start #####");

	for (int i = 1; i <= context->stepsSize; i++) {
		PipelineStep* step = &context->steps[i - 1];

		lua_geti(L, -1, i);
		errno_t err = getStep(L, step);
		if(err != 0) {
			LOG_ERROR("Error while getting step: %d", err);
			err = -1;
			goto _cleanup;
		}

		LOG_INFO("Step id: %s", step->id);
		LOG_INFO("Step handler ref: %d", step->handlerRef);
		lua_pop(L, 1);
	}


	LOG_TRACE("##### Pipeline building end #####");

	return 0;
_cleanup: 
	closePipeline(L, context);
	return err;
}

errno_t closePipeline(lua_State* L, PipelineContext* context) {
	for(int i = 0; i < context->stepsSize; i++) {
		PipelineStep* step = &context->steps[i];
		if(step == NULL) {
			continue;
		}

		luaL_unref(L, LUA_REGISTRYINDEX, step->handlerRef);
	}

	lua_close(L);
	return 0;
}

// the whole pipeline expect the seriliazation and deserialization steps of the request / response
// are implemented in lua for starter

// the pipeline is a linked list of steps defined by a full userdata in lua
// the handler functions as parameters: 
//	- a table as input containing generic info about the step
//  - a function to call to pass the result to the next step

#pragma region PipelineContextRequestLua



#pragma endregion

#pragma region PipelineContextLua


static int context_next(lua_State* L);
static int context_handlerequest(lua_State* L);

static const struct luaL_Reg pipeline_m[] = {
	{"next", context_next},
	{"handleRequest", context_handlerequest},
	{NULL, NULL}
};

static int context_next(lua_State* L) {
	PipelineContext* context = (PipelineContext*) luaL_checkudata(L, 1, PIPELINE_STEP_ARGS_META);

	context->currentStep += 1;
	if(context->currentStep >= context->stepsSize) {
		LOG_INFO("Pipeline ended");
		return 0;
	}

	PipelineStep* step = &context->steps[context->currentStep];

	lua_rawgeti(L, LUA_REGISTRYINDEX, step->handlerRef);	// push function
	lua_insert(L, -2);	// move usedata before function

	int err = lua_pcall(L, 1, 0, 0);
	if (err != 0) {
		LOG_ERROR("error running function: %s", lua_tostring(L, -1));
		return 0;
	}

	return 0;
}

static int context_handlerequest(lua_State* L) {
	PipelineContext* context = (PipelineContext*) luaL_checkudata(L, 1, PIPELINE_STEP_ARGS_META);

	Config* config = context->config;
	HttpRequest* request = context->request;
	HttpResponse* response = context->response;
	size_t routesize = context->routesSize;
	Route* routes = context->routes;

	errno_t err = 0;
	err = handleError(config, request, response);
	if (err == 1) {
		err = handleRequest(routes, routesize, config, request, response);
		if(err != 0) {
			LOG_ERROR("Error while handling request: %d", err);
			luaL_error(L, "Error while handling request: %d", err);
			return 0;
		}
	}

	return 0;
}

#pragma endregion

#pragma region LuaLogger 

static int l_lua_line(lua_State* L) {
	lua_Debug ar;
	lua_getstack(L, 1, &ar);
	lua_getinfo(L, "nSl", &ar);
	return ar.currentline;
}

static int l_log_trace(lua_State* L) {
	const char* message = luaL_checkstring(L, 1);
	LOG_TRACE_R("%s", "LUA", l_lua_line(L), message);
	return 0;
}

static int l_log_info(lua_State* L) {
	const char* message = luaL_checkstring(L, 1);
	LOG_INFO_R("%s", "LUA", l_lua_line(L), message);
	return 0;
}


static int l_log_warn(lua_State* L) {
	const char* message = luaL_checkstring(L, 1);
	LOG_WARN_R("%s", "LUA", l_lua_line(L), message);
	return 0;
}

static int l_log_error(lua_State* L) {
	const char* message = luaL_checkstring(L, 1);
	LOG_ERROR_R("%s", "LUA", l_lua_line(L), message);
	return 0;
}

static int l_log_fatal(lua_State* L) {
	const char* message = luaL_checkstring(L, 1);
	LOG_FATAL_R("%s", "LUA", l_lua_line(L), message);
	return 0;
}

static const struct luaL_Reg logger_f[] = {
	{"trace", l_log_trace},
	{"info", l_log_info},
	{"warn", l_log_warn},
	{"error", l_log_error},
	{"fatal", l_log_fatal},
	{NULL, NULL}
};

#pragma endregion

static int luaopen_pipeline(lua_State* L) {
	luaL_newlib(L, pipeline_m);			// create a new table and register functions
	lua_setglobal(L, "pipeline");		// set the table as global

	luaL_newlib(L, logger_f);			// create a new table and register functions
	lua_setglobal(L, "log");			// set the table as global

	luaL_newmetatable(L, PIPELINE_STEP_ARGS_META);
	lua_pop(L, 1);

	return 1;
}

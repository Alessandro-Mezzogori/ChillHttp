#include <chill_pipeline.h>

#define PIPELINE_STEP_META "chill.pipeline"
#define PIPELINE_CONTEXT_META "chill.pipeline.context"
#define PIPELINE_STEP_ARGS_META "chill.pipeline.stepargs"

#define PIPELINE_ROUTES_META "chill.routes"
#define PIPELINE_ROUTES_ROUTE_META "chill.routes.route"
#define PIPELINE_CONFIG_META "chill.config"

typedef struct RoutesWrapper {
	Route* routes;
	size_t size;
} RoutesWrapper;

typedef struct RouteWrapper {
	Route* route;
} RouteWrapper;

typedef struct ConfigWrapper {
	Config* config;
} ConfigWrapper;

errno_t runPipeline(PipelineContextInit* init) {
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

	RoutesWrapper* wrapper = (RoutesWrapper*) lua_newuserdatauv(L, sizeof(RoutesWrapper), init->routesSize);
	wrapper->routes = init->routes;
	wrapper->size = init->routesSize;

	luaL_getmetatable(L, PIPELINE_ROUTES_META);
	lua_setmetatable(L, -2);

	for(int i = 0; i < init->routesSize; i++) {
		Route* route = &init->routes[i];
		RouteWrapper* routeWrapper = (RouteWrapper*) lua_newuserdata(L, sizeof(RouteWrapper));
		routeWrapper->route = route;
		luaL_getmetatable(L, PIPELINE_ROUTES_ROUTE_META);
		lua_setmetatable(L, -2);
		lua_setiuservalue(L, -2, i + 1);
	}

	lua_setglobal(L, "routes");

	ConfigWrapper* configWrapper = (ConfigWrapper*) lua_newuserdata(L, sizeof(ConfigWrapper));
	configWrapper->config = init->config;
	luaL_getmetatable(L, PIPELINE_CONFIG_META);
	lua_setmetatable(L, -2);
	lua_setglobal(L, "config");

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

static const struct luaL_Reg pipeline_arg_m[] = {
	{NULL, NULL}
};

#pragma endregion

#pragma region LuaConfig

static int config_index(lua_State* L);
static int config_newindex(lua_State* L);

static const struct luaL_Reg config_f[] = {
	{"__index", config_index},
	{"__newindex", config_newindex},
	{NULL, NULL}
};

static int config_index(lua_State* L){
	ConfigWrapper* wrapper = (ConfigWrapper*) luaL_checkudata(L, 1, PIPELINE_CONFIG_META);
	Config* config = wrapper->config;

	const char* key = luaL_checkstring(L, -1);

	if(strcmp(key, "port") == 0) {
		lua_pushstring(L, config->port);
	}
	else if(strcmp(key, "servingFolder") == 0) {
		lua_pushstring(L, config->servingFolder);
	}
	else if(strcmp(key, "servingFolderLen") == 0) {
		lua_pushinteger(L, config->servingFolderLen);
	}
	else if(strcmp(key, "recvTimeout") == 0) {
		lua_pushinteger(L, config->recvTimeout);
	}
	else if(strcmp(key, "maxConcurrentThreads") == 0) {
		lua_pushinteger(L, config->maxConcurrentThreads);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

static int config_newindex(lua_State* L) {
	luaL_error(L, "Config table is read only");
	return 0;
}

#pragma endregion

#pragma region PipelineContextLua

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

static const struct luaL_Reg pipeline_f[] = {
	{"next", context_next},
	{"handleRequest", context_handlerequest},
	{NULL, NULL}
};


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

#pragma region LuaRoutes

static const int route_index(lua_State* L);

static const struct luaL_Reg route_m[] = {
	{"__index", route_index},
	{NULL, NULL}
};

static const int route_index(lua_State* L) {
	RouteWrapper* wrapper = (RouteWrapper*) luaL_checkudata(L, 1, PIPELINE_ROUTES_ROUTE_META);
	const char* key = luaL_checkstring(L, -1);

	if(strcmp(key, "method") == 0) {
		// TODO translate method to string, expose method in http.c
		lua_pushinteger(L, wrapper->route->method);
	}
	else if(strcmp(key, "path") == 0) {
		lua_pushstring(L, wrapper->route->route);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

static const int routes_size(lua_State* L);
static const int routes_index(lua_State* L);
static const int routes_newindex(lua_State* L);

static const struct luaL_Reg routes_f[] = {
	{"__len", routes_size},
	{"__index", routes_index},
	{"__newindex", routes_newindex},
	{NULL, NULL}
};

static const int routes_newindex(lua_State* L) {
	luaL_error(L, "Route table is read only");
	return 0;
}

static const int routes_index(lua_State* L) {
	RoutesWrapper* wrapper = (RoutesWrapper*) luaL_checkudata(L, 1, PIPELINE_ROUTES_META);

	int indexType = lua_type(L, -1);
	if (indexType == LUA_TSTRING) {
		const char* key = luaL_checkstring(L, -1);
		size_t route_f_size = sizeof(routes_f) / sizeof(struct luaL_Reg);
		for(int i = 0; i < route_f_size; i++) {
			if(strcmp(routes_f[i].name, key) == 0) {
				lua_pushcfunction(L, routes_f[i].func);
				return 1;
			}
		}
	}
	else if (indexType == LUA_TNUMBER) {
		const int index = luaL_checkinteger(L, -1);
		luaL_argcheck(L, 0 < index && index <= wrapper->size, 2, "index out of range");

		lua_getiuservalue(L, -2, index);
	}

	return 1;
}

static const int routes_size(lua_State* L) {
	RoutesWrapper* wrapper = (RoutesWrapper*) luaL_checkudata(L, 1, PIPELINE_ROUTES_META);
	lua_pushinteger(L, wrapper->size);
	return 1;
}

#pragma endregion

static int luaopen_pipeline(lua_State* L) {
	luaL_newlib(L, pipeline_f);			// create a new table and register functions
	lua_setglobal(L, "pipeline");		// set the table as global

	luaL_newlib(L, logger_f);			// create a new table and register functions
	lua_setglobal(L, "log");			// set the table as global

	luaL_newmetatable(L, PIPELINE_STEP_ARGS_META);
	luaL_setfuncs(L, pipeline_arg_m, 0);
	lua_pop(L, 1);

	luaL_newmetatable(L, PIPELINE_ROUTES_META);
	luaL_setfuncs(L, routes_f, 0);
	lua_pop(L, 1);

	luaL_newmetatable(L, PIPELINE_ROUTES_ROUTE_META);
	luaL_setfuncs(L, route_m, 0);
	lua_pop(L, 1);

	luaL_newmetatable(L, PIPELINE_CONFIG_META);
	luaL_setfuncs(L, config_f, 0);
	lua_pop(L, 1);

	return 1;
}

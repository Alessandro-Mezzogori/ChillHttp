#include <chill_pipeline.h>

errno_t closePipeline(lua_State* L, ContextWrapper* context);
errno_t setupPipeline(lua_State* L, ContextWrapper* context);

#pragma region lua_setup

void* lua_newuserdatauv_with_meta(lua_State* L, size_t size, int nuvalue, const char* meta) {
	void* data = lua_newuserdatauv(L, size, nuvalue);
	luaL_getmetatable(L, meta);
	lua_setmetatable(L, -2);
	return data;
}

#pragma endregion

errno_t runPipeline(PipelineContextInit* init) {
	// setup context
	// call first step

	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	lua_openlibs_pipeline(L);

	int r = luaL_dofile(L, PIPELINE_SCRIPT);
	if (r != LUA_OK) {
		LOG_ERROR("Error: %s", lua_tostring(L, -1));
		lua_close(L);
		return -1;
	}

	// ##### Routes global setup ##### 
	RoutesWrapper* wrapper = (RoutesWrapper*) lua_newuserdatauv_with_meta(L, sizeof(RoutesWrapper), (int) init->routesSize, PIPELINE_ROUTES_META);	
	wrapper->routes = init->routes;
	wrapper->size = init->routesSize;

	for(int i = 0; i < init->routesSize; i++) {
		Route* route = &init->routes[i];

		RouteWrapper* routeWrapper = (RouteWrapper*) lua_newuserdatauv_with_meta(L, sizeof(RouteWrapper), 0, PIPELINE_ROUTES_ROUTE_META); 
		routeWrapper->route = route;
		lua_setiuservalue(L, -2, i + 1); 
	}

	lua_setglobal(L, "routes"); 
	// ##### Routes global setup ##### 


	// ##### Config global setup ##### 
	ConfigWrapper* configWrapper = (ConfigWrapper*) lua_newuserdatauv_with_meta(L, sizeof(ConfigWrapper), 0, PIPELINE_CONFIG_META);
	configWrapper->config = init->config;
	lua_setglobal(L, "config");
	// ##### Config global setup ##### 

	// ##### Context.request setup #####
	HttpRequestWrapper* requestWrapper = (HttpRequestWrapper*) lua_newuserdatauv_with_meta(L, sizeof(HttpRequestWrapper), 1, PIPELINE_HTTPREQUEST_META);
	requestWrapper->request = init->request;

	HashTableWrapper* requestHeaderWrapper = (HashTableWrapper*) lua_newuserdatauv_with_meta(L, sizeof(HashTableWrapper), 0, PIPELINE_HASHTABLE_META);
	requestHeaderWrapper->ht = init->request->headers;
	requestHeaderWrapper->isReadOnly = true;
	lua_setiuservalue(L, -2, HTTPREQUEST_HEADER_INDEX);
	// ##### Context.request setup #####

	// ##### Context.response setup #####
	HttpResponseWrapper* responseWrapper = (HttpResponseWrapper*) lua_newuserdatauv_with_meta(L, sizeof(HttpResponseWrapper), 1, PIPELINE_HTTPRESPONSE_META);
	responseWrapper->response = init->response;

	HashTableWrapper* responseHeaderWrapper = (HashTableWrapper*) lua_newuserdatauv(L, sizeof(HashTableWrapper), 0, PIPELINE_HASHTABLE_META);
	responseHeaderWrapper->ht = init->response->headers;
	responseHeaderWrapper->isReadOnly = false;
	lua_setiuservalue(L, -2, HTTPRERESPONSE_HEADER_INDEX);
	// ##### Context.response setup #####

	// ##### Context.connection setup #####
	ConnectionDataWrapper* connectionWrapper = (ConnectionDataWrapper*) lua_newuserdatauv_with_meta(L, sizeof(HttpResponseWrapper), 0, PIPELINE_CONNECTIONDATA_META);
	connectionWrapper->connectionData = init->connectionData;
	// ##### Context.connection setup #####

	// ##### Context and steps setup #####
	lua_len(L, -4);
	int tableLen = (int) lua_tointeger(L, -1);
	LOG_TRACE("Pipeline table length: %d", tableLen);
	lua_pop(L, 1);


	size_t nbytes = sizeof(ContextWrapper) + tableLen*sizeof(PipelineLuaStep);
	ContextWrapper* a = (ContextWrapper*) lua_newuserdatauv_with_meta(L, nbytes, 3, PIPELINE_STEP_ARGS_META);
	a->request = init->request;
	a->response = init->response;
	a->connectionData = init->connectionData;
	a->config = init->config;

	a->routes = init->routes;
	a->routesSize = init->routesSize;
	a->stepsSize = tableLen;
	a->currentStep = 0;

	for(int i = 0; i < tableLen; i++) {
		PipelineLuaStep* step = &a->steps[i];
		step->handlerRef = LUA_NOREF;
		step->id[0] = 0;
	}

	lua_insert(L, -2);	// move usedata before function)
	lua_setiuservalue(L, -2, PIPELINE_CONTEXT_CONNECTION_INDEX);

	lua_insert(L, -2);	// move usedata before function)
	lua_setiuservalue(L, -2, PIPELINE_CONTEXT_RESPONSE_INDEX);

	lua_insert(L, -2);	// move usedata before function)
	lua_setiuservalue(L, -2, PIPELINE_CONTEXT_REQUEST_INDEX);

	lua_insert(L, -2);	// move usedata before function)
	setupPipeline(L, a);
	lua_insert(L, -2);	// move usedata before function)
	// ##### Context and steps setup #####

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

errno_t getStep(lua_State* L, PipelineLuaStep* step) {
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

errno_t setupPipeline(lua_State*L, ContextWrapper* context) {
	errno_t err = 0;

	LOG_TRACE("##### Pipeline building start #####");

	for (int i = 1; i <= context->stepsSize; i++) {
		PipelineLuaStep* step = &context->steps[i - 1];

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

errno_t closePipeline(lua_State* L, ContextWrapper* context) {
	for(int i = 0; i < context->stepsSize; i++) {
		PipelineLuaStep* step = &context->steps[i];
		if(step == NULL) {
			continue;
		}

		luaL_unref(L, LUA_REGISTRYINDEX, step->handlerRef);
	}

	lua_close(L);
	return 0;
}

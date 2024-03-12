#include <chill_pipeline.h>

errno_t closePipeline(lua_State* L, ContextWrapper* context);
errno_t setupPipeline(lua_State* L, ContextWrapper* context);

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

	RoutesWrapper* wrapper = (RoutesWrapper*) lua_newuserdatauv(L, sizeof(RoutesWrapper), (int) init->routesSize);
	wrapper->routes = init->routes;
	wrapper->size = init->routesSize;

	luaL_getmetatable(L, PIPELINE_ROUTES_META);
	lua_setmetatable(L, -2);

	for(int i = 0; i < init->routesSize; i++) {
		Route* route = &init->routes[i];
		RouteWrapper* routeWrapper = (RouteWrapper*) lua_newuserdatauv(L, sizeof(RouteWrapper), 0);
		routeWrapper->route = route;
		luaL_getmetatable(L, PIPELINE_ROUTES_ROUTE_META);
		lua_setmetatable(L, -2);
		lua_setiuservalue(L, -2, i + 1);
	}

	lua_setglobal(L, "routes");

	ConfigWrapper* configWrapper = (ConfigWrapper*) lua_newuserdatauv(L, sizeof(ConfigWrapper), 0);
	configWrapper->config = init->config;
	luaL_getmetatable(L, PIPELINE_CONFIG_META);
	lua_setmetatable(L, -2);
	lua_setglobal(L, "config");

	HttpRequestWrapper* requestWrapper = (HttpRequestWrapper*) lua_newuserdatauv(L, sizeof(HttpRequestWrapper), 1);
	requestWrapper->request = init->request;
	luaL_getmetatable(L, PIPELINE_HTTPREQUEST_META);
	lua_setmetatable(L, -2);

	HashTableWrapper* requestHeaderWrapper = (HashTableWrapper*) lua_newuserdatauv(L, sizeof(HashTableWrapper), 0);
	requestHeaderWrapper->ht = init->request->headers;
	requestHeaderWrapper->isReadOnly = true;
	luaL_getmetatable(L, PIPELINE_HASHTABLE_META);
	lua_setmetatable(L, -2);
	lua_setiuservalue(L, -2, HTTPREQUEST_HEADER_INDEX);

	HttpResponseWrapper* responseWrapper = (HttpResponseWrapper*) lua_newuserdatauv(L, sizeof(HttpResponseWrapper), 1);
	responseWrapper->response = init->response;
	luaL_getmetatable(L, PIPELINE_HTTPRESPONSE_META);
	lua_setmetatable(L, -2);

	HashTableWrapper* responseHeaderWrapper = (HashTableWrapper*) lua_newuserdatauv(L, sizeof(HashTableWrapper), 0);
	responseHeaderWrapper->ht = init->response->headers;
	responseHeaderWrapper->isReadOnly = false;
	luaL_getmetatable(L, PIPELINE_HASHTABLE_META);
	lua_setmetatable(L, -2);
	lua_setiuservalue(L, -2, HTTPRERESPONSE_HEADER_INDEX);

	ConnectionDataWrapper* connectionWrapper = (ConnectionDataWrapper*) lua_newuserdatauv(L, sizeof(HttpResponseWrapper), 0);
	connectionWrapper->connectionData = init->connectionData;
	luaL_getmetatable(L, PIPELINE_CONNECTIONDATA_META);
	lua_setmetatable(L, -2);

	lua_len(L, -4);
	int tableLen = (int) lua_tointeger(L, -1);
	LOG_TRACE("Pipeline table length: %d", tableLen);
	lua_pop(L, 1);


	size_t nbytes = sizeof(ContextWrapper) + tableLen*sizeof(PipelineLuaStep);
	ContextWrapper* a = (ContextWrapper*) lua_newuserdatauv(L, nbytes, 3);
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

	luaL_getmetatable(L, PIPELINE_STEP_ARGS_META);
	lua_setmetatable(L, -2);

	lua_insert(L, -2);	// move usedata before function)
	lua_setiuservalue(L, -2, PIPELINE_CONTEXT_CONNECTION_INDEX);

	lua_insert(L, -2);	// move usedata before function)
	lua_setiuservalue(L, -2, PIPELINE_CONTEXT_RESPONSE_INDEX);

	lua_insert(L, -2);	// move usedata before function)
	lua_setiuservalue(L, -2, PIPELINE_CONTEXT_REQUEST_INDEX);

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

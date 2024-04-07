#include <chill_pipeline_lualibs.h>


#pragma region PipelineHashtable

static int hashtable_index(lua_State* L);
static int hashtable_newindex(lua_State* L);

static const struct luaL_Reg readonly_hashtable_m[] = {
	{"__index", hashtable_index},
	{"__newindex", hashtable_newindex},
	{NULL, NULL}
};

static int hashtable_newindex(lua_State* L) {
	HashTableWrapper* wrapper = (HashTableWrapper*) luaL_checkudata(L, 1, PIPELINE_HASHTABLE_META);

	if(wrapper->isReadOnly) {
		luaL_error(L, "Hashtable is read only");
		return 0;
	}

	stackDump(L);
	const char* key = luaL_checkstring(L, -2);
	const char* value = luaL_checkstring(L, -1);
	hashtableAdd(wrapper->ht, key, value);

	return 0;
}

static int hashtable_index(lua_State* L) {
	HashTableWrapper* wrapper = (HashTableWrapper*) luaL_checkudata(L, 1, PIPELINE_HASHTABLE_META);
	const char* key = luaL_checkstring(L, -1);

	hashtablePrint(stdout, wrapper->ht);

	HashEntry* value = hashtableLookup(wrapper->ht, key);

	if(value != NULL) {
		lua_pushstring(L, value->value);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

#pragma endregion

#pragma region PipelineHttpRequestLua

static int request_index(lua_State* L);
static int request_newindex(lua_State* L);

static const struct luaL_Reg http_request_m[] = {
	{"__index", request_index},
	{"__newindex", request_newindex},
	{NULL, NULL}
};

static request_index(lua_State* L) {
	HttpRequestWrapper* wrapper = (HttpRequestWrapper*) luaL_checkudata(L, 1, PIPELINE_HTTPREQUEST_META);
	HttpRequest* request = wrapper->request;

	const char* key = luaL_checkstring(L, -1);

	if(strcmp(key, "method") == 0) {
		// TODO translate method to string, expose method in http.c
		lua_pushinteger(L, request->method);
	}
	else if(strcmp(key, "path") == 0) {
		lua_pushstring(L, request->path);
	}
	else if(strcmp(key, "version") == 0) {
		// TODO translate version to string, expose version in http.c
		lua_pushinteger(L, request->version);
	}
	else if(strcmp(key, "body") == 0) {
		// TODO translate version to string, expose version in http.c
		lua_pushstring(L, request->body);
	}
	else if(strcmp(key, "headers") == 0) {
		lua_getiuservalue(L, -2, HTTPREQUEST_HEADER_INDEX);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

static int request_newindex(lua_State* L) {
	luaL_error(L, "Request table is read only");
	return 0;
}

#pragma endregion

#pragma region PipelineHttpResponseLua

static int response_index(lua_State* L);
static int response_newindex(lua_State* L);

static const struct luaL_Reg http_response_m[] = {
	{"__index", response_index},
	{"__newindex", response_newindex},
	{NULL, NULL}
};

static response_newindex(lua_State* L) {
	HttpResponseWrapper* wrapper = (HttpResponseWrapper*) luaL_checkudata(L, 1, PIPELINE_HTTPRESPONSE_META);
	HttpResponse* response = wrapper->response;

	const char* key = luaL_checkstring(L, -2);

	if(strcmp(key, "version") == 0) {
		int version = (int) luaL_checkinteger(L, -1);
		if(version < 1 || version > 3) {
			luaL_error(L, "Invalid version: %d", version);
		}

		response->version = (HTTP_VERSION) version;
	}
	else if(strcmp(key, "status") == 0) {
		unsigned short statuscode = (unsigned short) luaL_checkinteger(L, -1);
		if (statuscode < 100 || statuscode >= 600) {
			luaL_error(L, "Invalid status code: %d", statuscode);
		}

		response->statusCode = statuscode;
	}
	else if(strcmp(key, "body") == 0) {
		const char* body = luaL_checkstring(L, -1);

		if(response->body != NULL) {
			free(response->body);
		}

		setHttpResponseBody(response, body);
	}
	else if(strcmp(key, "headers") == 0) {
		lua_getiuservalue(L, -2, HTTPRERESPONSE_HEADER_INDEX);
	}

	return 0;
}

static int response_index(lua_State* L) {
	HttpResponseWrapper* wrapper = (HttpResponseWrapper*) luaL_checkudata(L, 1, PIPELINE_HTTPRESPONSE_META);
	HttpResponse* response = wrapper->response;

	const char* key = luaL_checkstring(L, -1);

	if(strcmp(key, "version") == 0) {
		// TODO translate version to string, expose version in http.c
		lua_pushinteger(L, response->version);
	}
	else if(strcmp(key, "statusCode") == 0) {
		lua_pushinteger(L, response->statusCode);
	}
	else if(strcmp(key, "body") == 0) {
		lua_pushstring(L, response->body);
	}
	else if(strcmp(key, "headers") == 0) {
		lua_getiuservalue(L, -2, HTTPRERESPONSE_HEADER_INDEX);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

#pragma endregion

#pragma region PipelineConnectionDataLua

static int connection_index(lua_State* L);
static int connection_newindex(lua_State* L);
static int connection_close(lua_State* L);

static const struct luaL_Reg connection_m[] = {
	{"__index",		connection_index},
	{"__newindex",	connection_newindex},
	{"close",		connection_close},
	{NULL, NULL}
};

static connection_newindex(lua_State* L) {
	luaL_error(L, "Connection table is read only");
	return 0;
}

static int connection_index(lua_State* L) {
	ConnectionDataWrapper* wrapper = (ConnectionDataWrapper*) luaL_checkudata(L, 1, PIPELINE_CONNECTIONDATA_META);
	ConnectionData* connection = wrapper->connectionData;

	const char* key = luaL_checkstring(L, -1);

	size_t connection_m_size = sizeof(connection_m) / sizeof(struct luaL_Reg);
	for(int i = 0; i < connection_m_size; i++) {
		if(connection_m[i].name != NULL && strcmp(connection_m[i].name, key) == 0) {
			lua_pushcfunction(L, connection_m[i].func);
			return 1;
		}
	}

	if(strcmp(key, "status") == 0) {
		// TODO translate version to string, expose version in http.c
		lua_pushinteger(L, connection->connectionStatus);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

static int connection_close(lua_State* L) {
	ConnectionDataWrapper* wrapper = (ConnectionDataWrapper*) luaL_checkudata(L, 1, PIPELINE_CONNECTIONDATA_META);
	ConnectionData* connection = wrapper->connectionData;

	connection->connectionStatus = CONNECTION_STATUS_CLOSING;
	return 0;
}

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

static int route_index(lua_State* L);

static const struct luaL_Reg route_m[] = {
	{"__index", route_index},
	{NULL, NULL}
};

static int route_index(lua_State* L) {
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

static int routes_size(lua_State* L);
static int routes_index(lua_State* L);
static int routes_newindex(lua_State* L);

static const struct luaL_Reg routes_f[] = {
	{"__len", routes_size},
	{"__index", routes_index},
	{"__newindex", routes_newindex},
	{NULL, NULL}
};

static int routes_newindex(lua_State* L) {
	luaL_error(L, "Route table is read only");
	return 0;
}

static int routes_index(lua_State* L) {
	RoutesWrapper* wrapper = (RoutesWrapper*) luaL_checkudata(L, 1, PIPELINE_ROUTES_META);

	int indexType = lua_type(L, -1);
	if (indexType == LUA_TSTRING) {
		const char* key = luaL_checkstring(L, -1);
		size_t route_f_size = sizeof(routes_f) / sizeof(struct luaL_Reg);
		for(int i = 0; i < route_f_size; i++) {
			if(routes_f[i].name != NULL && strcmp(routes_f[i].name, key) == 0) {
				lua_pushcfunction(L, routes_f[i].func);
				return 1;
			}
		}
	}
	else if (indexType == LUA_TNUMBER) {
		int index = (int) luaL_checkinteger(L, -1);
		luaL_argcheck(L, 0 < index, 2, "index out of range");

		// TODO find way to do a better check without casting 
		luaL_argcheck(L, 0 < (size_t) index && index <= wrapper->size, 2, "index out of range"); 

		lua_getiuservalue(L, -2, index);
	}

	return 1;
}

static int routes_size(lua_State* L) {
	RoutesWrapper* wrapper = (RoutesWrapper*) luaL_checkudata(L, 1, PIPELINE_ROUTES_META);
	lua_pushinteger(L, wrapper->size);
	return 1;
}

#pragma endregion

#pragma region PipelineContextArgs 

static int args_index(lua_State* L);

static const struct luaL_Reg args_m[] = {
	{"__index", args_index},
	{NULL, NULL}
};

static int args_index(lua_State* L) {
	ContextWrapper* context = (ContextWrapper*) luaL_checkudata(L, 1, PIPELINE_STEP_ARGS_META);
	const char* key = luaL_checkstring(L, -1);

	size_t pipeline_f_size = sizeof(args_m) / sizeof(struct luaL_Reg);
	for(int i = 0; i < pipeline_f_size; i++) {
		if(args_m[i].name != NULL && strcmp(args_m[i].name, key) == 0) {
			lua_pushcfunction(L, args_m[i].func);
			return 1;
		}
	}

	if (strcmp(key, "request") == 0) {
		lua_getiuservalue(L, -2, PIPELINE_CONTEXT_REQUEST_INDEX);
	}
	else if (strcmp(key, "response") == 0) {
		lua_getiuservalue(L, -2, PIPELINE_CONTEXT_RESPONSE_INDEX);
	}
	else if (strcmp(key, "connection") == 0) {
		lua_getiuservalue(L, -2, PIPELINE_CONTEXT_CONNECTION_INDEX);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

#pragma endregion

#pragma region PipelineContextLua

static int pipeline_next(lua_State* L);
static int pipeline_handlerequest(lua_State* L);

static const struct luaL_Reg pipeline_f[] = {
	{"next", pipeline_next},
	{"handleRequest", pipeline_handlerequest},
	{NULL, NULL}
};

static int pipeline_next(lua_State* L) {
	ContextWrapper* context = (ContextWrapper*) luaL_checkudata(L, 1, PIPELINE_STEP_ARGS_META);

	context->currentStep += 1;
	if(context->currentStep >= context->stepsSize) {
		LOG_INFO("Pipeline ended");
		return 0;
	}

	PipelineLuaStep* step = &context->steps[context->currentStep];

	lua_rawgeti(L, LUA_REGISTRYINDEX, step->handlerRef);	// push function
	lua_insert(L, -2);	// move usedata before function

	int err = lua_pcall(L, 1, 0, 0);
	if (err != 0) {
		LOG_ERROR("error running function: %s", lua_tostring(L, -1));
		return 0;
	}

	return 0;
}

static int pipeline_handlerequest(lua_State* L) {
	ContextWrapper* context = (ContextWrapper*) luaL_checkudata(L, 1, PIPELINE_STEP_ARGS_META);

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

int lua_openlibs_pipeline(lua_State* L) {
	luaL_newlib(L, logger_f);			// create a new table and register functions
	lua_setglobal(L, "log");			// set the table as global

	luaL_newmetatable(L, PIPELINE_ROUTES_META);
	luaL_setfuncs(L, routes_f, 0);
	lua_pop(L, 1);

	luaL_newmetatable(L, PIPELINE_ROUTES_ROUTE_META);
	luaL_setfuncs(L, route_m, 0);
	lua_pop(L, 1);

	luaL_newmetatable(L, PIPELINE_CONFIG_META);
	luaL_setfuncs(L, config_f, 0);
	lua_pop(L, 1);

	luaL_newmetatable(L, PIPELINE_HASHTABLE_META);
	luaL_setfuncs(L, readonly_hashtable_m, 0);
	lua_pop(L, 1);

	luaL_newmetatable(L, PIPELINE_HTTPREQUEST_META);
	luaL_setfuncs(L, http_request_m, 0);
	lua_pop(L, 1);

	luaL_newmetatable(L, PIPELINE_HTTPRESPONSE_META);
	luaL_setfuncs(L, http_response_m, 0);
	lua_pop(L, 1);

	luaL_newmetatable(L, PIPELINE_CONNECTIONDATA_META);
	luaL_setfuncs(L, connection_m, 0);
	lua_pop(L, 1);

	luaL_newlib(L, pipeline_f);			// create a new table and register functions
	lua_setglobal(L, "pipeline");		// set the table as global

	luaL_newmetatable(L, PIPELINE_STEP_ARGS_META);
	luaL_setfuncs(L, args_m, 0);
	lua_pop(L, 1);

	return 1;
}

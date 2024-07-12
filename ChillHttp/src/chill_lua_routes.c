#include <chill_lua_routes.h>

struct LuaRoute {
	Route* routes;
	size_t num;
};


void parse_lua_routes(lua_State* L, void* data) {
	lua_getglobal(L, "conf");
	struct LuaRoute* luaRoute = (struct LuaRoute*) data;

	if(lua_istable(L, -1)){
		// get num of routes

		// loop trough routes
		// parse trough route
		// TODO router function to call lua route ( needs to save the handler and be in the same lua state has the computations )

		getfield(L, "PORT", LUA_TYPE_STRING, conf->port, MAX_CONFIG_PORT_LEN);
		getfield(L, "MAX_CONCURRENT_THREADS", LUA_TYPE_SIZE_T, &conf->maxConcurrentThreads, 0);
		getfield(L, "SERVING_FOLDER", LUA_TYPE_STRING, conf->servingFolder, MAX_SERVING_FOLDER_LEN);

		size_t timeout;
		getfield(L, "RECV_TIMEOUT", LUA_TYPE_SIZE_T, &timeout, 0);
		conf->recvTimeout = timeout * 1000;

		conf->servingFolderLen = strlen(conf->servingFolder);

		getfield(L, "PIPELINE_FILE_PATH", LUA_TYPE_STRING, conf->pipelineFilePath, CHILL_CONFIG_FILEPATH_LEN);
		getfield(L, "ROUTES_FILE_PATH", LUA_TYPE_STRING, conf->routesFilePath, CHILL_CONFIG_FILEPATH_LEN);
	}
	else {
		LOG_ERROR("conf is not a table");
	}
}

errno_t chill_load_routes(const char* str, Route** routes, size_t* routenum){
	struct LuaRoute luaRoute;
	errno_t err = launch_lua_script_standalone(str, parse_lua_routes, &luaRoute);

	*routes = luaRoute.routes;
	*routenum = luaRoute.num;

	return err;
}

#include <chill_config.h>


void parse_lua_conf(lua_State* L, void* data) {
	lua_getglobal(L, "conf");
	Config* conf = (Config*) data;

	if(lua_istable(L, -1)){
		getfield(L, "PORT", LUA_TYPE_STRING, conf->port, 6);
		getfield(L, "MAX_CONCURRENT_THREADS", LUA_TYPE_SIZE_T, &conf->maxConcurrentThreads, 0);
		getfield(L, "SERVING_FOLDER", LUA_TYPE_STRING, conf->servingFolder, 256);

		size_t timeout;
		getfield(L, "RECV_TIMEOUT", LUA_TYPE_SIZE_T, &timeout, 0);
		conf->recvTimeout = timeout * 1000;

		conf->servingFolderLen = strlen(conf->servingFolder);
	}
	else {
		LOG_ERROR("conf is not a table");
	}
}

errno_t chill_load_config(const char* str, Config* config) {
	return launch_lua_script_standalone(str, parse_lua_conf, config);
}

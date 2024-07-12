#include <chill_config.h>


void parse_lua_conf(lua_State* L, void* data) {
	lua_getglobal(L, "conf");
	Config* conf = (Config*) data;

	if(lua_istable(L, -1)){
		getfield(L, "PORT", LUA_TYPE_STRING, conf->port, MAX_CONFIG_PORT_LEN);
		getfield(L, "MAX_CONCURRENT_THREADS", LUA_TYPE_SIZE_T, &conf->maxConcurrentThreads, 0);
		getfield(L, "SERVING_FOLDER", LUA_TYPE_STRING, conf->servingFolder, MAX_SERVING_FOLDER_LEN);

		size_t timeout;
		getfield(L, "RECV_TIMEOUT", LUA_TYPE_SIZE_T, &timeout, 0);
		conf->recvTimeout = timeout * 1000;

		conf->servingFolderLen = strlen(conf->servingFolder);

		getfield(L, "PIPELINE_FILE_PATH", LUA_TYPE_STRING, conf->pipelineFilePath, MAX_PIPELINE_FILEPATH_LEN);
	}
	else {
		LOG_ERROR("conf is not a table");
	}
}

errno_t chill_load_config(const char* str, Config* config) {
	return launch_lua_script_standalone(str, parse_lua_conf, config);
}

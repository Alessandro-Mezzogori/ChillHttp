#include <chill_lua.h>

errno_t CheckLua(lua_State* L, int r) {
	if (r != LUA_OK) {
		LOG_ERROR("Error: %s", lua_tostring(L, -1));
		return r;
	}

	return 0;
}

void launch_lua_script_standalone(const char* script, lua_callback callback, void* data) {
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	int r = luaL_dofile(L, script);
	if (r != LUA_OK) {
		LOG_ERROR("Error: %s", lua_tostring(L, -1));
	}

	callback(L, data);

	lua_close(L);
}

errno_t getfield(lua_State* L, const char* key, LuaType type, void* dest) {
	lua_pushstring(L, key);
	lua_gettable(L, -2); // TODO make it configurable

	switch (type)
	{
		case LUA_TYPE_SIZE_T: 
			*(size_t*)dest = lua_tointeger(L, -1);
			break;
		case LUA_TYPE_INT: 
			*(int*)dest = lua_tointeger(L, -1);
			break;
		case LUA_TYPE_NUMBER:
			*(double*)dest = lua_tonumber(L, -1);
			break;
		case LUA_TYPE_STRING:
			strcpy((char*)dest, lua_tostring(L, -1));
			break;
		default:
			break;
	}

	lua_pop(L, 1);

	return 0;
}
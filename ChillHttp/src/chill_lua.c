#include <chill_lua.h>

void stackDump(lua_State* L) {
	int i;
	int top = lua_gettop(L);
	LOG_TRACE("##### LUA STACK DUMP #####");
	for (i = 1; i <= top; i++) {  /* repeat for each level */
		int t = lua_type(L, i);
		switch (t) {
			case LUA_TSTRING:  /* strings */
				LOG_TRACE("%d `%s'", i, lua_tostring(L, i));
				break;

			case LUA_TBOOLEAN:  /* booleans */
				LOG_TRACE("%d %s", i, lua_toboolean(L, i) ? "true" : "false");
				break;

			case LUA_TNUMBER:  /* numbers */
				LOG_TRACE("%d %g", i, lua_tonumber(L, i));
				break;

			default:  /* other values */
				LOG_TRACE("%d %s", i, lua_typename(L, t));
				break;
		}
	}
}

errno_t CheckLua(lua_State* L, int r) {
	if (r != LUA_OK) {
		LOG_ERROR("Error: %s", lua_tostring(L, -1));
		return r;
	}

	return 0;
}

errno_t launch_lua_script_standalone(const char* script, lua_callback callback, void* data) {
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	int r = luaL_dofile(L, script);
	if (r != LUA_OK) {
		LOG_ERROR("Error: %s", lua_tostring(L, -1));
		return r;
	}

	callback(L, data);
	lua_close(L);
	return 0;
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
			*(int*)dest = (int) lua_tointeger(L, -1);
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
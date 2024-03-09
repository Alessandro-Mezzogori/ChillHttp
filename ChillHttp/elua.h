#pragma once

#include "log/log.h"

#include "lua542/include/lua.h"
#include "lua542/include/lauxlib.h"
#include "lua542/include/luaconf.h"
#include "lua542/include/lualib.h"
#pragma comment(lib, "lua542/lua54.lib")

typedef void (lua_callback)(lua_State* L, void* data);

typedef enum {
	LUA_TYPE_INT,
	LUA_TYPE_SIZE_T,
	LUA_TYPE_NUMBER,
	LUA_TYPE_STRING,
	LUA_TYPE_TABLE,
} LuaType;

void launch_lua_script_standalone(const char* script, lua_callback callback, void* data);
errno_t getfield(lua_State* L, const char* key, LuaType type, void* dest);


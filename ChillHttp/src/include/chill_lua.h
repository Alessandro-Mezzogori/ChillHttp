#pragma once

#include <lua.h>
#include <lauxlib.h>
#include <luaconf.h>
#include <lualib.h>

#include <chill_log.h>

#pragma comment(lib, "lua54.lib")

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


#pragma once

#include <chill_lua.h>
#include <stdlib.h>
#include <limits.h>


typedef struct BitArray {
  int size;
  unsigned int values[1];  /* variable part */
} BitArray;

int luaopen_array(lua_State* L);


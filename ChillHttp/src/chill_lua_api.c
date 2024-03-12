#include <chill_lua_boolean_array.h>

#define BITS_PER_WORD 32
#define I_WORD(i) ((unsigned)i / BITS_PER_WORD)
#define I_BIT(i) (1 << ((unsigned)i % BITS_PER_WORD))

#define META_NAME "chill.array"
#define checkarray(L) (BitArray*)luaL_checkudata(L, 1, META_NAME)

static int newarray(lua_State* L) {
	size_t nbytes;
	BitArray *a;

	int n = (int) luaL_checkinteger(L, 1);
	luaL_argcheck(L, n >= 1, 1, "invalid size");
	nbytes = sizeof(BitArray) + I_WORD(n - 1) * sizeof(unsigned int);
	a = (BitArray*) lua_newuserdata(L, nbytes);

	a->size = n;
	for(unsigned i = 0; i <= I_WORD(n - 1); i++) 
		a->values[i] = 0;  /* initialize array */

	luaL_getmetatable(L, META_NAME);
	lua_setmetatable(L, -2);

	return 1;
}

static unsigned int* getparams(lua_State* L, unsigned int* mask) {
	BitArray* a = checkarray(L);
	int index = (int) luaL_checkinteger(L, 2) - 1; // starts from 1 in lua

	luaL_argcheck(L, 0 <= index && index < a->size, 2, "index out of range");
	*mask = I_BIT(index);
	return &a->values[I_WORD(index)];
}

static int setarray(lua_State* L) {
	unsigned int mask;
	unsigned int* entry = getparams(L, &mask);

	luaL_checkany(L, 3);
	if (lua_toboolean(L, 3)) {
		*entry |= mask;
	}
	else {
		*entry &= ~mask;
	}

	return 0;
}

static int getarray(lua_State* L) {
	unsigned int mask;
	unsigned int *entry = getparams(L, &mask);
	lua_pushboolean(L, *entry & mask);	

	return 1;
}

static int getsize(lua_State* L) {
	BitArray* a = checkarray(L);
	lua_pushinteger(L, a->size);
	return 1;
}

static int array2string(lua_State* L) {
	BitArray* a = checkarray(L);
	lua_pushfstring(L, "array(%d)", a->size);
	return 1;
}

static const struct luaL_Reg arraylib_f[] = {
	{"new", newarray},
	{NULL, NULL}
};

static const struct luaL_Reg arraylib_m[] = {
	{"set", setarray},
	{"get", getarray},
	{"size", getsize},
	{"__tostring", array2string},
	{"__index", getarray},
	{"__newindex", setarray},
	{"__len", getsize},
	{NULL, NULL}
};

int luaopen_array(lua_State* L) {
	luaL_newmetatable(L, META_NAME);	// creates metatable for chill.array
	luaL_setfuncs(L, arraylib_m, 0);	// register methods

	luaL_newlib(L, arraylib_f);
	lua_setglobal(L, "array");

	return 1;
}

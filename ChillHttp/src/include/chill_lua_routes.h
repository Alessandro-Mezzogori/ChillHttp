#pragma once

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <chill_lua.h>
#include <chill_router.h>

errno_t chill_load_routes(const char* str, Route* routes, size_t* routenum);


#pragma once

#include <chill_log.h>

typedef struct _ChillLock ChillLock;

ChillLock* lockInit();
void monitorEnter(ChillLock* lock);
void monitorExit(ChillLock* lock);
void lockFree(ChillLock* lock);

#pragma once

#include <chill_log.h>

typedef struct _ChillLock ChillLock;

void lockInit(ChillLock* lock);
void monitorEnter(ChillLock* lock);
void monitorExit(ChillLock* lock);
void lockFree(ChillLock* lock);

#include "chill_lock.h"
#include <windows.h>

struct _ChillLock {
	CRITICAL_SECTION cs;
};

void lockInit(ChillLock* lock) {
	InitializeCriticalSection(&lock->cs);
}

void monitorEnter(ChillLock* lock) {
	EnterCriticalSection(&lock->cs);
}

void monitorExit(ChillLock* lock) {
	LeaveCriticalSection(&lock->cs);
}

void lockFree(ChillLock* lock) {
	DeleteCriticalSection(&lock->cs);
}

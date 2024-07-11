#include "chill_lock.h"
#include <windows.h>

struct _ChillLock {
	CRITICAL_SECTION cs;
};

ChillLock* lockInit() {
	// TODO error handleing
	ChillLock* mallock = malloc(sizeof(ChillLock));
	if (mallock == NULL) {
		return;
	}

	InitializeCriticalSection(&mallock->cs);
	return mallock;
}

void monitorEnter(ChillLock* lock) {
	EnterCriticalSection(&lock->cs);
}

void monitorExit(ChillLock* lock) {
	LeaveCriticalSection(&lock->cs);
}

void lockFree(ChillLock* lock) {
	DeleteCriticalSection(&lock->cs);
	free(lock);
}

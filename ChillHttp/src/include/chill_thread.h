#pragma once

// TODO: remove depencency from windows header, prefer a notation with implemenation files per platform like chill_thread.windows.c
// should be done after working prototype and exploration of problem space
#include <chill_log.h>
#include <stdbool.h>

typedef enum _ChillThreadState {
	ThreadRunning			= 0,
	ThreadStopRequested		= 1,	// thread is blocked - wait, sleep, join ....
	ThreadBackground		= 2,
	ThreadUnstarted			= 4,	// start state
	ThreadStop				= 8,
	ThreadWaitSleepJoin		= 16,
	ThreadAbortRequested	= 32,
	ThreadAborted			= 64
} ChillThreadState;

typedef struct _ChillThread ChillThread;
typedef struct _ChillThreadInit {
	errno_t(*work)(void* data);
	void* data;
	bool delayStart;
} ChillThreadInit;

errno_t chill_thread_init(ChillThreadInit* init, ChillThread** threadptr);
errno_t chill_thread_start(ChillThread* thread);
errno_t chill_thread_join(ChillThread* thread, unsigned long ms);
//errno_t chill_thread_cancel(ChillThread* thread);
errno_t chill_thread_abort(ChillThread* thread); 
//errno_t chill_thread_stop(ChillThread* thread);
errno_t chill_thread_cleanup(ChillThread* thread);

ChillThreadState chill_thread_getstate(ChillThread* thread);
size_t chill_thread_getid(ChillThread* thread);
void* chill_thread_getdata(ChillThread* data);


// Monitor and lock usage 
// https://source.dot.net/#System.Private.CoreLib/src/System/Threading/Monitor.CoreCLR.cs,79752c0abaf2f34c
// Info about locks and monitor in c# 
// https://learn.microsoft.com/en-us/dotnet/api/system.threading.monitor.enter?view=netframework-4.7

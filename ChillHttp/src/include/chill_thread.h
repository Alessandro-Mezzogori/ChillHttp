#pragma once

// TODO: remove depencency from windows header, prefer a notation with implemenation files per platform like chill_thread.windows.c
// should be done after working prototype and exploration of problem space
#include <windows.h>

typedef enum ChillThreadRequest {
	RequestNone		= 0,
	RequestCancel	= 1,
	RequestStop		= 2,
} ChillThreadRequest;

typedef enum ChillThreadState {
	ThreadRunning			= 0,
	ThreadStopRequested		= 1,	// thread is blocked - wait, sleep, join ....
	ThreadBackground		= 2,
	ThreadUnstarted			= 4,	// start state
	ThreadStop				= 8,
	ThreadWaitSleepJoin		= 16,
	ThreadAbortRequested	= 32,
	ThreadAborted			= 64
} ChillThreadState;

// 
typedef errno_t(*ChillThreadPWF)(void* data);

typedef struct ChillThread {
	unsigned int _id;
	HANDLE _hndl;	// TODO remove HANDLE type for a platform agnostic implementation
	ChillThreadState _state;
	CRITICAL_SECTION

	errno_t (*work)(void* data);
	void* data;
} ChillThread;

errno_t chill_thread_init(ChillThread* thread);
errno_t chill_thread_start(ChillThread* thread);
errno_t chill_thread_join(ChillThread* thread);
errno_t chill_thread_cancel(ChillThread* thread);
errno_t chill_thread_abort(ChillThread* thread);
errno_t chill_thread_stop(ChillThread* thread);


// Monitor and lock usage 
// https://source.dot.net/#System.Private.CoreLib/src/System/Threading/Monitor.CoreCLR.cs,79752c0abaf2f34c
// Info about locks and monitor in c# 
// https://learn.microsoft.com/en-us/dotnet/api/system.threading.monitor.enter?view=netframework-4.7

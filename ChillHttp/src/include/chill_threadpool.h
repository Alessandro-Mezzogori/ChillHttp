#pragma once
#include <windows.h>
#include <chill_log.h>

typedef enum ChillThreadRequest {
	RequestNone		= 0,
	RequestCancel	= 1,
	RequestStop		= 2,
} ChillThreadRequest;

typedef enum ChillThreadState {
	ThreadRunning		= 0,
	ThreadIdle			= 1,	// thread is blocked - wait, sleep, join ....
	ThreadCancelRequested = 2,
	ThreadCancelled		= 4,
	ThreadAbort			= 8,
	ThreadStopRequested = 16,
	ThreadStopped		= 32,
} ChillThreadState;

typedef enum ChillTaskState {
	TaskCreated,
	TaskScheduled,
	TaskRunning,
	TaskCompleted,
	TaskAborted,
	TaskCancelled, // TODO support cancellation token like
} ChillTaskState;

typedef struct ChillTask {
	// Work function 
	errno_t (*work_function)(void* data);

	// Data passed to the work function 
	void* data;

	// State of the task
	ChillTaskState state;

	// Result of the task, ownership is of who destroys the task ( evaluate the use of an optional cleanup function )
	void* result;
} ChillTask;

typedef struct ChillThread {
	// Pool management
	// updated by pool | read from thread
	unsigned int m_id;
	HANDLE m_hndl;

	// request from the thread pool
	ChillThreadRequest m_request; // TODO QUEUE of requests ? 

	// Lets the thread sleep when there's no work allocated for it
	CONDITION_VARIABLE m_awake;
	CRITICAL_SECTION m_lock;

	// TODO TaskQueue for each thread
	ChillTask* m_task;

	// Thread management
	// read from pool | updated by thread 
	ChillThreadState t_state;
} ChillThread;

typedef struct ChillThreadPool {
	size_t threadSize;
	ChillThread* threads;
} ChillThreadPool;

// Thread pool needs a function pointer to be generic and allow diverse uses instead of a task runner like right now
errno_t chill_threadpool_init(ChillThreadPool* pool, int threadNumber);
errno_t chill_threadpool_free(ChillThreadPool* pool);
ChillThread* chill_threadpool_getfreethread(ChillThreadPool* pool);

// get thread info
// assign work to thread
// get thread pool info

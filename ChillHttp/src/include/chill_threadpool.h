#pragma once
#include <windows.h>
#include <chill_thread.h>
#include <chill_log.h>

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

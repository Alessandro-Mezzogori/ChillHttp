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


typedef void (*ChillTaskWork)(void* data);
typedef struct ChillTaskInit {
	// Work function 
	ChillTaskWork work;

	// Data passed to the work function 
	void* data;
} ChillTaskInit;

typedef struct _ChillThreadPool ChillThreadPool;
typedef struct _ChillTask ChillTask;

typedef struct _ChillThreadPoolInit {
	unsigned short min_thread;
	unsigned short max_thread;
} ChillThreadPoolInit;

// Thread pool needs a function pointer to be generic and allow diverse uses instead of a task runner like right now
errno_t chill_threadpool_init(ChillThreadPool** pool, ChillThreadPoolInit* init);
errno_t chill_threadpool_free(ChillThreadPool* pool);

ChillTask* chill_task_create(ChillThreadPool* pool, ChillTaskInit* init);
errno_t chill_task_submit(ChillTask* task);
errno_t chill_task_free(ChillTask* task);
errno_t chill_task_wait(ChillTask* task);
errno_t chill_task_wait_multiple(ChillTask* task, size_t num);

// get thread info
// assign work to thread
// get thread pool info

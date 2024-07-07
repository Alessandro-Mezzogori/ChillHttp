/// Using thread pool functinos
///	https://learn.microsoft.com/en-us/windows/win32/procthread/using-the-thread-pool-functions
/// Threadpool winapi
///	https://learn.microsoft.com/en-us/windows/win32/procthread/thread-pool-api

#include "chill_threadpool.h"

struct _ChillThreadPool {
	TP_CALLBACK_ENVIRON callbackEnv;
	PTP_POOL pool;
	PTP_CLEANUP_GROUP cleanupgroup;

	DWORD max_threads;
	DWORD min_threads;
};

struct _ChillTask {
	ChillTaskWork cbk;
	void* data;
	PTP_WORK work;
};

errno_t chill_threadpool_init_validate(ChillThreadPoolInit* init) {
	if (init == NULL) {
		return 1;
	}

	if (init->max_thread <= init->min_thread) {
		LOG_ERROR("ChillthreadPoolInit: Max thread must be higher than min thread");
		return 2;
	}

	return 0;
}

errno_t chill_threadpool_init(ChillThreadPool** pool, ChillThreadPoolInit* init) {
	TP_CALLBACK_ENVIRON callbackEnv;
	PTP_POOL threadpool = NULL;
	PTP_CLEANUP_GROUP cleanupgroup = NULL;
	size_t rollback = 0;
	errno_t init_validation = 0;

	init_validation = chill_threadpool_init_validate(init);
	if (init_validation != 0) {
		LOG_ERROR("ChillthreadPoolInit: validation failed");
		goto _cleanup;
	}

	InitializeThreadpoolEnvironment(&callbackEnv);
	threadpool = CreateThreadpool(NULL);
	if (threadpool == NULL) {
		LOG_ERROR("CreateThreadPool failed. LastError: %u", GetLastError()); 
		goto _cleanup;
	}
	rollback = 1;	// pool creation success

	SetThreadpoolThreadMaximum(threadpool, init->max_thread);
	BOOL minimum_set_result = SetThreadpoolThreadMinimum(threadpool, init->min_thread);
	if (minimum_set_result == FALSE) {
		LOG_ERROR("setThreadPoolMinimum failed, Last Error %u", GetLastError());
		goto _cleanup;
	}

	cleanupgroup = CreateThreadpoolCleanupGroup();
	if (cleanupgroup == NULL) {
		LOG_ERROR("CreateThreadCleanupGroup failed, Last Error %u", GetLastError());
		goto _cleanup;
	}
	rollback = 2; // cleanup group creation success

	SetThreadpoolCallbackPool(&callbackEnv, threadpool);
	SetThreadpoolCallbackCleanupGroup(&callbackEnv, cleanupgroup, NULL);

	ChillThreadPool* chillpool = malloc(sizeof(ChillThreadPool));
	if (chillpool == NULL) {
		LOG_ERROR("ChillThreadPool allocation failed, Last Error");
		goto _cleanup;
	}

	chillpool->callbackEnv = callbackEnv;
	chillpool->pool = threadpool;
	chillpool->cleanupgroup = cleanupgroup;
	chillpool->max_threads = init->max_thread;
	chillpool->min_threads = init->min_thread;

	*pool = chillpool;
	return 0;

_cleanup:
	switch (rollback)
	{
	case 2:
		CloseThreadpoolCleanupGroup(cleanupgroup);
	case 1:
		CloseThreadpool(threadpool);
	default:
		break;
	}

	return rollback;
}

errno_t chill_threadpool_free(ChillThreadPool* pool) {
	CloseThreadpoolCleanupGroupMembers(pool->cleanupgroup, FALSE, NULL);
	CloseThreadpoolCleanupGroup(pool->cleanupgroup);
	CloseThreadpool(pool->pool);
	DestroyThreadpoolEnvironment(&pool->callbackEnv);

	pool->cleanupgroup = NULL;
	pool->pool = NULL;

	free(pool);

	return 0;
}

void windows_thread_work(PTP_CALLBACK_INSTANCE instance, PVOID* context, PTP_WORK work) {
	ChillTask* task = (ChillTask*) context;

	task->cbk(task->data);

	chill_task_free(task);
}

ChillTask* chill_task_create(ChillThreadPool* pool, ChillTaskInit* init) {
	ChillTask* task = NULL;
	PTP_WORK work = 0;

	task = malloc(sizeof(ChillTask));
	if (task == NULL) {
		LOG_ERROR("ChillTask allocation failed");
		goto _cleanup;
	}

	work = CreateThreadpoolWork(
		windows_thread_work, 
		task, 
		&pool->callbackEnv
	);
	if (work == NULL) {
		LOG_ERROR("CreateThreadpoolWork failed, last error: %u", GetLastError());
		goto _cleanup;
	}

	task->cbk = init->work;
	task->data = init->data;
	task->work = work;

	return task;
_cleanup:
	free(task);
	return NULL;
}

errno_t chill_task_submit(ChillTask* task) {
	if (task == NULL) {
		LOG_WARN("ChillTask* task should not be null");
		return 1;
	}

	SubmitThreadpoolWork(task->work);
	return 0;
}

errno_t chill_task_free(ChillTask* task) {
	if (task == NULL) {
		LOG_WARN("Passing null task to free");
		return 0;
	}

	free(task);
	return 0;
}

errno_t chill_task_wait(ChillTask* task) {
	WaitForThreadpoolWorkCallbacks(task->work, FALSE);
	return 0;
}

errno_t chill_task_wait_multiple(ChillTask** tasks, size_t num) {
	for (size_t i = 0; i < num; i++) {
		chill_task_wait(tasks[i]);
	}
}

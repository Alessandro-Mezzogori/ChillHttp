#include "chill_threadpool.h"

errno_t chill_threadpool_init(ChillThreadPool* pool, int threadNumber) {
	if (pool == NULL) {
		LOG_WARN("Pool is non instantieted");
		return 1;
	}

	if (threadNumber <= 0 && threadNumber > 10) {
		LOG_WARN("Invalid argument: threadNumber is not a positive number less or equal to 10");
		return 1;
	}

	pool->threadSize = threadNumber;
	pool->threads = malloc(threadNumber * sizeof(ChillThread));
	for (int i = 0; i < threadNumber; ++i) {
		ChillThread thread = pool->threads[i];

		InitializeConditionVariable(&thread.m_hasWork);
		InitializeCriticalSection(&thread.m_threadLock);

		thread.m_request = None;
		thread.t_state = Running;
		thread.m_hndl = CreateThread(
			NULL,					// default security
			0,						// default stack size
			chill_thread_function,	// name of thread function
			&thread,				// thread parameters
			0,						// default startup flags
			&thread.t_state // CreateThread output id
		);
	}

	return 0;
}

errno_t chill_threadpool_free(ChillThreadPool* pool) {
	for (int i = 0; i < pool->threadSize; ++i) {
		ChillThread thread = pool->threads[i];

		WakeConditionVariable(&thread.m_hasWork);

		if (thread.m_hndl != NULL) {
			WaitForSingleObject(thread.m_hndl, INFINITE); // TODO join timeout for threadpool cleanup
		}

		DeleteCriticalSection(&thread.m_threadLock);
	}

	free(pool->threads);
}

DWORD chill_thread_function(void* lpParam) {
	ChillThread* thread = (ChillThread*)lpParam;

	int exit = 0;
	LOG_DEBUG("Thread %d start", thread->m_id);
	while(!exit) {
		EnterCriticalSection(&thread->m_threadLock);

		while (thread->m_task == NULL && _ThreadStopRequested(thread) == FALSE) {
			thread->t_state = WaitingWork;
			SleepConditionVariableCS(
				&thread->m_hasWork,
				&thread->m_threadLock,
				INFINITE // TODO sleep condition timeout
			);
		}

		if (_ThreadStopRequested(thread) == TRUE) {
			thread->t_state = CancelRequested;
			LeaveCriticalSection(&thread->m_threadLock);
			thread->t_state = Cancelled;
			break;
		}

		// DO WORK
		thread->t_state = Running;
		if (thread->m_task != NULL) {
			ChillTask* task = thread->m_task;
			task->work_function(task->data);
		}

		LeaveCriticalSection(&thread->m_threadLock);
	}

	return 0;
}

int _ThreadStopRequested(ChillThread* thread) { return thread->m_request == Cancel; }

#include "chill_threadpool.h"

/*
errno_t chill_threadpool_free(ChillThreadPool* pool) {
	for (int i = 0; i < pool->threadSize; ++i) {
		ChillThread* thread = &pool->threads[i];

		LOG_DEBUG("Wake %ld", thread->m_id);

		thread->m_request = RequestStop;
		WakeConditionVariable(&thread->m_awake);

		if (thread->m_hndl != NULL) {
			LOG_DEBUG("Wait %ld", thread->m_id);
			DWORD waitResult = WaitForSingleObject(thread->m_hndl, INFINITE); // TODO join timeout for threadpool cleanup
			LOG_DEBUG("Wait result %ul", waitResult);
		}

		LOG_DEBUG("Delete critical section %ld", thread->m_id);
	}

	free(pool->threads);
	return 0;
}

int _ThreadCancelRequested(ChillThread* thread) {  
	if (thread->m_request == RequestCancel) {
		thread->t_state = ThreadSuspendRequested;
		return TRUE;
	}

	return FALSE;
}

int _ThreadStopRequested(ChillThread* thread) {  
	if (thread->m_request == RequestStop) {
		thread->t_state = ThreadStopRequested;
		return TRUE;
	}

	return FALSE;
}

int _ThreadGenericCloseRequested(ChillThread* thread) {
	return _ThreadCancelRequested(thread) || _ThreadStopRequested(thread);
}

DWORD chill_threadpool_thread_function(void* lpParam) {
	ChillThread* thread = (ChillThread*)lpParam;

	int exit = 0;
	LOG_DEBUG("Thread %lu start", thread->m_id);
	while(!exit) {
		LOG_DEBUG("Enter critical section %lu", thread->m_id);
		EnterCriticalSection(&thread->m_lock);

		// check if thread has work to do.
		while (_ThreadGenericCloseRequested(thread) == FALSE) {
			thread->t_state = ThreadStopRequested;
			SleepConditionVariableCS(
				&thread->m_awake,
				&thread->m_lock,
				INFINITE // TODO sleep condition timeout
			);
		}

		LOG_DEBUG("out loop %lu", thread->m_id);
		if (_ThreadGenericCloseRequested(thread) == TRUE) {
			if (thread->t_state == ThreadSuspendRequested) {
				LOG_DEBUG("Thread %lu cancelled", thread->m_id);
				thread->t_state = ThreadBackground;
			}
			else if (thread->t_state == ThreadStopRequested) {
				LOG_DEBUG("Thread %lu stopped", thread->m_id);
				thread->t_state = ThreadStopped;
			}

			LeaveCriticalSection(&thread->m_lock);
			break;
		}

		// DO WORK
		LOG_DEBUG("Thread %lu running", thread->m_id);
		thread->t_state = ThreadRunning;
		if (thread->m_task != NULL) {
			ChillTask* task = thread->m_task;
			task->work_function(task->data);
		}
		LOG_DEBUG("Thread %lu finished running", thread->m_id);
		LeaveCriticalSection(&thread->m_lock);
	}

	return 0;
}

errno_t _chill_thread_init(ChillThread* thread) {
	InitializeConditionVariable(&thread->m_awake);
	InitializeCriticalSection(&thread->m_lock);

	thread->m_request = RequestNone;
	thread->m_task = NULL;
	thread->t_state = ThreadRunning;
	thread->m_hndl = CreateThread(
		NULL,					// default security
		0,						// default stack size
		chill_threadpool_thread_function,	// name of thread function
		thread,					// thread parameters
		0,						// default startup flags
		&thread->m_id			// CreateThread output id
	);

	if (thread->m_hndl == NULL) {	
		// TODO abstract dword from lib code
		DWORD error = GetLastError();
		LOG_ERROR("Thread creation gone wrong: Error Code %", error);
		return error;

	}

	return 0;
}

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
	if (pool->threads == NULL) {
		// TODO handle malloc error
		return 1;
	}

	for (int i = 0; i < threadNumber; ++i) {
		_chill_thread_init(&pool->threads[i]);
	}

	return 0;
}

// TODO thread safe lock on chill thread modification, is it necessary to have ChillThreads.ops thread safe, tbf they should be held by a master thread that dispatches the work and then joins it. 
// the Pointers to ChillThread should not be shared into multiple threads but just between the parent-child thread for communication purposes,
// Signal thread to stop when possibile
errno_t _chill_thread_stop(ChillThread* thread) {
	thread->m_request = RequestStop;
	WakeConditionVariable(&thread->m_awake);
	return 0;
}

// naive, only for testing purposes
ChillThread* chill_threadpool_getfreethread(ChillThreadPool* pool) {
	for (int i = 0; i < pool->threadSize; ++i) {
		if (pool->threads[i].t_state == ThreadStopRequested) {
			return &pool->threads[i];
		}
	}

	return NULL;
}
*/

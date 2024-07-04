#include <chill_thread.h>
#include <windows.h>

#define EVENT_NUM 1

struct _ChillThread {
	DWORD id;
	HANDLE hndl;	

	ChillThreadState state;

	// TODO: find another way to do this 
	CRITICAL_SECTION criticalSection;
	CONDITION_VARIABLE conditionalVariable;

	errno_t (*work)(void* data);
	void* data;
};

errno_t chill_thread_work(void* data) {
	ChillThread* thread = (ChillThread*)data;
	if (thread->work == NULL) {
		LOG_WARN("Spawned thread %ul without work function", thread->id);
		return 1;
	}

	errno_t workError = thread->work(thread->data);
	if (workError != 0) {
		LOG_WARN("Error in thread %u: Error Code %ul", thread->id, workError);
	}

	return workError;
	/*
	int exit = 0;
	while (!exit) {
		EnterCriticalSection(&thread->criticalSection);
		while (work == false) {
			SleepConditionVariableCS(
				&thread->conditionalVariable,
				&thread->criticalSection,
				INFINITE
			);
		}

		thread->state = ThreadRunning;
		LeaveCriticalSection(&thread->criticalSection);

		errno_t workError = thread->work(thread->data);
		if (workError != 0) {
			LOG_WARN("Error in thread %u: Error Code %ul", thread->id, workError);
		}
	}
	*/
}

errno_t chill_thread_init(ChillThreadInit* init, ChillThread** threadptr) {
	if (init == NULL) { 
		return 1;
	}

	if (init->work == NULL) {
		return 2;
	}

	ChillThread* thread = malloc(sizeof(ChillThread));
	if (thread == NULL) {
		return 3;
	}

	DWORD threadId = 0;
	HANDLE hdnl = CreateThread(
		NULL,
		0,
		chill_thread_work,
		thread,
		init->delayStart ? CREATE_SUSPENDED : 0,
		&threadId
	);

	if (hdnl == 0) {
		return GetLastError();
	}

	thread->id = threadId;
	thread->hndl = hdnl;
	thread->state = init->delayStart ? ThreadUnstarted : ThreadRunning;
	thread->work = init->work;	// required TODO check
	thread->data = init->data;	// optional
	InitializeCriticalSection(&thread->criticalSection);
	InitializeConditionVariable(&thread->conditionalVariable);

	*threadptr = thread;
	return 0;
_err:
	chill_thread_cleanup(thread);
	return 1;
}

errno_t chill_thread_start(ChillThread* thread) {
	EnterCriticalSection(&thread->criticalSection);
	thread->state = ThreadRunning;
	LeaveCriticalSection(&thread->criticalSection);

	DWORD result = ResumeThread(thread->hndl);

	return result != -1 ? 0 : 1;
}

errno_t chill_thread_join(ChillThread* thread, unsigned long ms) {
	EnterCriticalSection(&thread->criticalSection);
	thread->state = ThreadWaitSleepJoin;
	LeaveCriticalSection(&thread->criticalSection);

	WakeConditionVariable(&thread->conditionalVariable);

	DWORD result = WaitForSingleObject(thread->hndl, ms);
	if (result == WAIT_FAILED) {
		return GetLastError();
	}

	if (result == WAIT_TIMEOUT) {
		return 1; // TODO check error code is available
	}

	if (result == WAIT_ABANDONED) {
		return 2; // TODO check error code is available
	}

	return 0;
}

errno_t chill_thread_abort(ChillThread* thread) {
	TerminateProcess(&thread->hndl, 1);
	chill_thread_cleanup(thread);
}

ChillThreadState chill_thread_getstate(ChillThread* thread) {
	return thread->state;
}

size_t chill_thread_getid(ChillThread* thread) {
	return thread->id;
}

void* chill_thread_getdata(ChillThread* thread) {
	return thread->data;
}

/*
errno_t chill_thread_abort(ChillThread* thread) {

}
*/

errno_t chill_thread_cleanup(ChillThread* thread) {
	LOG_TRACE("Cleanup thread %u", thread->id);

	DeleteCriticalSection(&thread->criticalSection);

	if (thread->hndl != NULL) {
		CloseHandle(thread->hndl);
	}

	LOG_TRACE("Cleanup thread %u success", thread->id);
	return 0;
}

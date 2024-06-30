#include <chill_thread.h>

errno_t chill_thread_init(ChillThread* thread) {
	if (thread == NULL) { 
		return 1;
	}

	if (thread->work == NULL) {
		return 2;
	}

	DWORD threadId = 0;
	HANDLE hdnl = CreateThread(
		NULL,
		0,
		chill_thread_work,
		thread,
		CREATE_SUSPENDED,
		&threadId
	);

	if (hdnl == 0) {
		return GetLastError();
	}

	thread->_id = threadId;
	thread->_hndl = hdnl;
	thread->_state = ThreadUnstarted;
	return 0;
}

errno_t chill_thread_work(void* data) {
	ChillThread* thread = (ChillThread*)data;
}
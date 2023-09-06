#include "threading.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Optional: use these functions to add debug or error prints to your
// application
#define DEBUG_LOG(msg, ...)
// #define DEBUG_LOG(msg, ...) printf("threading: " msg "\n", ##__VA_ARGS__)
#define ERROR_LOG(msg, ...) printf("threading ERROR: " msg "\n", ##__VA_ARGS__)

void *threadfunc(void *thread_param) {
	// TODO: wait, obtain mutex, wait, release mutex as described by thread_data
	// structure hint: use a cast like the one below to obtain thread arguments
	// from your parameter struct thread_data* thread_func_args = (struct
	// thread_data *) thread_param;
	int ret;
	struct thread_data *tdata;

	tdata = (struct thread_data *)thread_param;
	usleep(tdata->wait_to_obtain_ms * 1000);
	ret = pthread_mutex_lock(tdata->mutex);
	if (ret) {
		ERROR_LOG("pthread_mutex_lock with ret = %d\n", ret);
		return thread_param;
	}

	usleep(tdata->wait_to_release_ms * 1000);
	ret = pthread_mutex_unlock(tdata->mutex);
	if (ret) {
		ERROR_LOG("pthread_mutex_unlock with ret = %d\n", ret);
		return thread_param;
	}
	tdata->thread_complete_success = true;
	return thread_param;
}

bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,
								  int wait_to_obtain_ms,
								  int wait_to_release_ms) {
	/**
	 * TODO: allocate memory for thread_data, setup mutex and wait arguments,
	 * pass thread_data to created thread using threadfunc() as entry point.
	 *
	 * return true if successful.
	 *
	 * See implementation details in threading.h file comment block
	 */
	int ret;
	struct thread_data *tdata;

	tdata = (struct thread_data *)malloc(sizeof(struct thread_data));
	tdata->mutex = mutex;
	tdata->wait_to_obtain_ms = wait_to_obtain_ms;
	tdata->wait_to_release_ms = wait_to_release_ms;
	tdata->thread_complete_success = false;

	ret = pthread_create(thread, NULL, threadfunc, (void *)tdata);
	if (ret) {
		free(tdata);
		ERROR_LOG("pthread_create with ret = %d\n", ret);
		return false;
	}

	return true;
}

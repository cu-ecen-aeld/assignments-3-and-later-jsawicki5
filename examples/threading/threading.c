#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
	struct thread_data * threadData = (struct thread_data *)thread_param;
	usleep(threadData->wait_to_obtain*1000);
	
	if(pthread_mutex_lock(threadData->t_mutex) != 0)
	{
		printf("pthread_mutex_lock failed");
	}
	else
	{
		usleep(threadData->wait_to_release*1000);
		threadData->thread_complete_success = true;
		
		if(pthread_mutex_unlock(threadData->t_mutex) != 0)
		{
			printf("pthread_mutex_unlock failed");
		}
		else
		{
			
		}
	}
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    return threadData;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
     struct thread_data * threadData = malloc(sizeof(struct thread_data));
	 
	 if(threadData != NULL)
	 {
		threadData->t_mutex = mutex;
		threadData->wait_to_obtain = wait_to_obtain_ms;
		threadData->wait_to_release = wait_to_release_ms;
		threadData->thread_complete_success = false;
		
		if(pthread_create(thread, NULL, threadfunc, threadData) != 0)
		{
			printf("pthread_create failed");
		}
		else
		{
			(void) threadData;
			return true;
		}
	}
	else
	{
		printf("failed malloc");
	}
	
    return false;
}


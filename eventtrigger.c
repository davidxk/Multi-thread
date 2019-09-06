#include <pthread.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>

struct timeval startTime;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

/* Register function */
int isEvent = 0;
const int NUM = 4;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
void registerFunction()
{
	pthread_mutex_lock( &m );
	printf("--> registered, %s\n", isEvent ? "go ahead!" : "wait ... ");
	/* The reason we need a variable `isEvent` to mark that the event as
	 * triggered, instead of just believing the event must have been triggered
	 * when we wake up from the `wait` call, is because `wait` could just
	 * return on its own even when no one has called `signal` or `broadcast`.
	 *
	 * In Golang, `Wait` cannot return unless awoken by `Signal` or `Broadcast`.
	 * In this case, there will be no need to have an additional variable.
	 */
	while(!isEvent)
		pthread_cond_wait(&cond, &m);
	pthread_mutex_unlock( &m );
}

/* Trigger function */
void* triggerEvent(void* arg)
{
	sleep(2);
	pthread_mutex_lock( &m );
	isEvent = 1;
	pthread_cond_broadcast( &cond );
	pthread_mutex_unlock( &m );
	pthread_exit(NULL);
}

/* Print simulation time */
void* print(void* arg)
{
	struct timeval nowTime, result;
	gettimeofday(&nowTime, NULL);
	timersub(&nowTime, &startTime, &result);
	long ltime = result.tv_sec * 1000000 + result.tv_usec;
	printf("  * start time: %ld\n", ltime);
	pthread_exit(NULL);
}

/* Synchronized version of print */
void* printSync(void* arg)
{
	registerFunction();
	print(NULL);
	pthread_exit(NULL);
}

/* Create four threads at different time and sync them */
void threadCreate(void* (func)(void*))
{
	gettimeofday(&startTime, NULL);

	pthread_t tid[NUM];
	int i = 0;
	for(i = 0; i < NUM; i++)
	{
		pthread_create(&tid[i], NULL, func, NULL);
		sleep(1);
	}
	for(i = 0; i < NUM; i++)
		pthread_join(tid[i], NULL);
}

/* Create the thread that triggers the event. Main thread joins every thing */
void triggerCreate(void* (func)(void*))
{
	pthread_t tid;
	pthread_create(&tid, NULL, func, NULL);
	pthread_detach(tid);
}

int main()
{
	printf("\n# Four threads start with barrier: \n");
	triggerCreate(triggerEvent);
	threadCreate(printSync);

	pthread_mutex_destroy( &m );
	pthread_mutex_destroy( &mtx );
	pthread_cond_destroy( &cond );
}

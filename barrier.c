#include <pthread.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>

struct timeval startTime;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

/* Standard Barrier Sync function */
int count = 0, generation = 0;
const int NUM = 3;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
void barrierSync()
{
	pthread_mutex_lock( &m );
	if(++count < NUM)
	{
		int myGeneration = generation;
		while(myGeneration == generation)
			pthread_cond_wait(&cond, &m);
	}
	else
	{
		count = 0;
		generation++;
		pthread_cond_broadcast( &cond );
	}
	pthread_mutex_unlock( &m );
}

/* Print simulation time */
void* print(void* arg)
{
	/*pthread_mutex_lock( &mtx );*/

	struct timeval nowTime, result;
	gettimeofday(&nowTime, NULL);
	timersub(&nowTime, &startTime, &result);
	long ltime = result.tv_sec * 1000000 + result.tv_usec;
	printf("  * start time: %ld\n", ltime);

	/*pthread_mutex_unlock( &mtx );*/
	pthread_exit(NULL);
}

/* Barriered version of print */
void* printSync(void* arg)
{
	barrierSync();
	print(NULL);
	pthread_exit(NULL);
}

#ifndef __APPLE__
pthread_barrier_t barr;
/* Barriered with Posix barrier */
void* printPosix(void* arg)
{
	pthread_barrier_wait( &barr );
	print(NULL);
}
#endif

/* Create three threads at different time and sync them */
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

int main()
{
	printf("# Three threads start without barrier: \n");
	threadCreate(print);
	printf("\n# Three threads start with barrier: \n");
	threadCreate(printSync);
#ifndef MAC
	printf("\n# Three threads start with POSIX barrier: \n");
	pthread_barrier_init(&barr, NULL, NUM);
	threadCreate(printPosix);
	pthread_barrier_destroy( &barr );
#endif

	pthread_mutex_destroy( &m );
	pthread_mutex_destroy( &mtx );
	pthread_cond_destroy( &cond );
}

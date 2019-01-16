/* 
 * reader()
 * {
 *    when(writers == 0) [
 *      readers++;
 *    ]
 *    // read here
 *    [readers--;]
 * }
 *
 * write()
 * {
 *    when(writers == 0 and readers == 0) [
 *      writers++;
 *    ]
 *    // write here
 *    [writers--;]
 * }
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int data[2] = { 0, 0 };

/* Reader function */
void* lire(void* arg)
{
	int* ptr = malloc(sizeof(int));
	*ptr = data[0] + data[1];
	return (void*)ptr;
}

/* Time-taking Writer function */
void* ecrire(void* arg)
{
	data[0] = 1;
	/* Carefully chosen sleep time may generate special result
	 * Choice of sleep time is machine dependent */
	usleep( 50 );
	data[1] = 1;

	return NULL;
}

/* Self-implemented reader lock */
int readers = 0, writers = 0;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t readersQ = PTHREAD_COND_INITIALIZER;
pthread_cond_t writersQ = PTHREAD_COND_INITIALIZER;

void* readerLocked(void* arg)
{
	pthread_mutex_lock( &m );
	while(writers > 0)
		pthread_cond_wait(&readersQ, &m);
	readers++;
	pthread_mutex_unlock( &m );

	void* retval = lire(NULL);

	pthread_mutex_lock( &m );
	if(--readers == 0)
		pthread_cond_signal( &writersQ );
	pthread_mutex_unlock( &m );
	return retval;
}

/* Self-implemented writer lock */
int active_writers = 0;
void* writerLocked(void* arg)
{
	pthread_mutex_lock( &m );
	writers++;
	while(readers > 0 && active_writers > 0)
		pthread_cond_wait(&writersQ, &m);
	active_writers++;
	pthread_mutex_unlock( &m );

	ecrire(NULL); 

	pthread_mutex_lock( &m );
	writers--;
	active_writers--;
	/* Prioritize writer threads */
	if(writers > 0)
		pthread_cond_signal( &writersQ );
	else
		pthread_cond_broadcast( &readersQ );
	pthread_mutex_unlock( &m );
	pthread_exit(NULL);
}

/* Posix reader lock */
pthread_rwlock_t rwl = PTHREAD_RWLOCK_INITIALIZER;
void* readerPosix(void* arg)
{
	pthread_rwlock_rdlock( &rwl );
	void* retval = lire(NULL);
	pthread_rwlock_unlock( &rwl );

	return retval;
}

/* Posix writer lock */
void* writerPosix(void* arg)
{
	pthread_rwlock_wrlock( &rwl );
	ecrire(NULL);
	pthread_rwlock_unlock( &rwl );
	pthread_exit(NULL);
}

/* Create 10 reader and writer threads to race */
void createWRThreads(void* (readFunc)(void*), void* (writeFunc)(void*))
{
	int i;
	const int NUM = 10;
	pthread_t tid[NUM];
	for(i = 0; i < NUM; i++)
	{
		if(i % 2 == 0)
			pthread_create(&tid[i], NULL, readFunc, NULL);
		else
			pthread_create(&tid[i], NULL, writeFunc, NULL);
	}
	for(i = 0; i < NUM; i++)
	{
		void* retval;
		pthread_join(tid[i], &retval);
		if(retval)
			printf("%d ", *(int*)retval);
	}
	printf("\n");
}

int main()
{
	printf("> 0: write not started\n> 1: write incomplete\n");
	printf("> 2: write complete\n");
	printf("> Note: different run might generate different result\n");
	printf("# Without read-write lock:\n");
	createWRThreads(lire, ecrire);
	printf("# With read-write lock:\n");
	createWRThreads(readerPosix, writerPosix);
	printf("# With POSIX read-write lock:\n");
	createWRThreads(readerLocked, writerLocked);

	pthread_rwlock_destroy( &rwl );
	pthread_mutex_destroy( &m );
	pthread_cond_destroy( &readersQ );
	pthread_cond_destroy( &writersQ );
	return 0;
}

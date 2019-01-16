/*
 * Guarded Command
 *   If the guard is true, the guard evaluation and command sequence execution
 * should be done in one atomic operation.  
 *
 *   when (guard) [
 *     command sequence
 *   ]
 *
 * Semaphore explained using Guarded Command
 * - P: when (S > 0) [ S-= 1 ]
 * - V: [ S+= 1 ]
 *
 * Semaphore is useful in Producer/Consumer problem and Reader/Writer problem
 * Fine grain parallelism control
 */


#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const int MAX = 20;
int conveyorbelt[MAX];

/* Self-implemented Semaphore P operation */
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
void P(int* sem)
{
	pthread_mutex_lock( &mtx );
	while(*sem <= 0)
		pthread_cond_wait( &cond, &mtx );
	(*sem)--;
	pthread_mutex_unlock( &mtx );
}

/* Self-implemented Semaphore V operation */
void V(int* sem)
{
	pthread_mutex_lock( &mtx );
	(*sem)++;
	pthread_cond_broadcast( &cond );
	pthread_mutex_unlock( &mtx );
}

/* Producer function */
int nextin = 0, nextout = 0;
void* produce(void* arg)
{
	if(arg != NULL)
		usleep( 100 );
	conveyorbelt[nextin] = 1;
	nextin = (nextin + 1) % MAX;
	return NULL;
}

/* Consumer function */
void* consume(void* arg)
{
	if(arg != NULL)
		usleep( 100 );
	int* retval = malloc(sizeof(int));
	*retval = conveyorbelt[nextout];
	conveyorbelt[nextout] = 0;
	nextout = (nextout + 1) % MAX;
	return retval;
}

/* Producer protected with self-implemented PV functions */
int empty = MAX, occup = 0;
pthread_mutex_t min = PTHREAD_MUTEX_INITIALIZER;
void* produceProtected(void* arg)
{
	/* Protect producer from full conveyorbelt */
	P(&empty);
	/* Protect producer from each other */
	pthread_mutex_lock( &min );
	produce(arg);
	pthread_mutex_unlock( &min );
	/* Notify consumer, another product is ready */
	V(&occup);

	return NULL;
}

/* Consumer protected with self-implemented PV functions */
pthread_mutex_t mout = PTHREAD_MUTEX_INITIALIZER;
void* consumeProtected(void* arg)
{
	P(&occup);
	/* Protect consumer from empty conveyorbelt */
	pthread_mutex_lock( &mout );
	/* Protect consumer from each other */
	void* retval = consume(arg);
	pthread_mutex_unlock( &mout );
	/* Notify producer, another slot is empty */
	V(&empty);

	return retval;
}

/* Producer protected with posix semaphore */
sem_t *emptySem, *occupSem;
void* producePosix(void* arg)
{
	sem_wait( emptySem );

	pthread_mutex_lock( &min );
	produce(arg);
	pthread_mutex_unlock( &min );

	sem_post( occupSem );
	return NULL;
}

/* Consumer protected with posix semaphore */
void* consumePosix(void* arg)
{
	sem_wait( occupSem );

	pthread_mutex_lock( &mout );
	void* retval = consume(arg);
	pthread_mutex_unlock( &mout );

	sem_post( emptySem );
	return retval;
}

static void helperFunc(void* (pFunc)(void*), void* (cFunc)(void*),
		void* first, void* second)
{
	int i;
	const int NTHREAD = 10;
	pthread_t tid[NTHREAD];

	for(i = 0; i < NTHREAD; i++)
		if(i%2 == 0)
			pthread_create(&tid[i], NULL, pFunc, first);
		else
			pthread_create(&tid[i], NULL, cFunc, second);
	for(i = 0; i < NTHREAD; i++)
		if(i%2 != 0)
		{
			void* retval;
			pthread_join(tid[i], &retval);
			printf("%d ", *(int*)retval);
		}
	printf("\n");
}

void readyGo(void* (pFunc)(void*), void* (cFunc)(void*))
{
	/* Fast consumer, slow producer, empty conveyor belt */
	helperFunc(pFunc, cFunc, (void*)50, NULL);
	/* Fast consumer, slow producer, conveyor belt not empty */
	int i;
	for(i = 0; i < 3; i++)
		pFunc(NULL);
	helperFunc(pFunc, cFunc, (void*)50, NULL);
	/* Fast producer, slow consumer */
	helperFunc(pFunc, cFunc, NULL, (void*)50);
}

int main()
{
	printf("> 0: consumer consuming NULL on conveyor belt\n");
	printf("> 1: consumer consuming valid item on conveyor belt\n");
	printf("> Note: different run might generate different result\n");
	printf("# Without using semaphore:\n");
	readyGo(produce, consume);
	printf("# With semaphore:\n");
	readyGo(produceProtected, consumeProtected);

	emptySem = sem_open("posix semaphore", O_CREAT, 0, MAX);
	occupSem = sem_open("posix semaphore", O_CREAT, 0, 0);
	printf("# With POSIX semaphore:\n");
	readyGo(producePosix, consumePosix);

	pthread_mutex_destroy( &mtx );
	pthread_cond_destroy( &cond );
	pthread_mutex_destroy( &min );
	pthread_mutex_destroy( &mout );
	return 0;
}

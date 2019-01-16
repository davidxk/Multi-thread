#include <condition_variable>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// Semaphore implementation
class Semaphore
{
public:
	Semaphore(int cnt = 0): counter(cnt) { }
	void p();
	void v();
private:
	int counter;
	std::mutex mtx;
	std::condition_variable cv;
};

void Semaphore::p()
{
	std::unique_lock<std::mutex> lck(mtx);
	while(counter <= 0)
		cv.wait(lck);
	counter--;
}

void Semaphore::v()
{
	std::unique_lock<std::mutex> lck(mtx);
	counter++;
	cv.notify_one();
}

// Producer Consumer without synchronization
class ProducerConsumer
{
public:
	ProducerConsumer();
	void produce();
	void consume();
protected:
	int nextIn, nextOut;
	const int MAX = 35;
	std::vector<int> conveyorbelt;
};

ProducerConsumer::ProducerConsumer(): nextIn(0), nextOut(0)
{
	conveyorbelt = std::vector<int>(MAX, 0);
}

void ProducerConsumer::produce()
{
	conveyorbelt[nextIn] = 1;
	nextIn = (nextIn + 1) % MAX;
}

void ProducerConsumer::consume()
{
	std::cout<<conveyorbelt[nextOut]<<" ";
	conveyorbelt[nextOut] = 0;
	nextOut = (nextOut + 1) % MAX;
}

// Producer Consumer with synchronization
class ProducerConsumerProtected: public ProducerConsumer
{
public:
	ProducerConsumerProtected(): nEmpty(MAX), nOccup() { }
	void produce();
	void consume();
private:
	std::mutex mtxIn, mtxOut;
	Semaphore nEmpty, nOccup;
};

void ProducerConsumerProtected::produce()
{
	nEmpty.p();
	mtxIn.lock();

	conveyorbelt[nextIn] = 1;
	nextIn = (nextIn + 1) % MAX;

	mtxIn.unlock();
	nOccup.v();
}

void ProducerConsumerProtected::consume()
{
	nOccup.p();
	mtxOut.lock();

	std::cout<<conveyorbelt[nextOut]<<" ";
	conveyorbelt[nextOut] = 0;
	nextOut = (nextOut + 1) % MAX;

	mtxOut.unlock();
	nEmpty.v();
}

int main()
{
	srand(time(nullptr));
	std::cout<<"Producer/Consumer problem"<<std::endl;
	std::cout<<"> 0: Empty\n> 1: Product"<<std::endl;
	std::cout<<"Each consumer print what it consummed with a space"<<std::endl;
	std::cout<<"- Producer Consumer unprotected"<<std::endl;
	ProducerConsumer pc;
	const int times = 70;
	std::thread tid[100];
	for(int i = 0; i < times; i++)
	{
		auto func = rand() % 2 ? &ProducerConsumer::produce : &ProducerConsumer::consume;
		tid[i] = std::thread(func, &pc);
	}
	for(int i = 0; i < times; i++)
		tid[i].join();
	std::cout<<std::endl;

	std::cout<<"- Producer Consumer protected"<<std::endl;
	ProducerConsumerProtected pcp;
	for(int i = 0; i < times; i++)
	{
		auto func = rand() % 3 > 0 ? &ProducerConsumerProtected::produce : &ProducerConsumerProtected::consume;
		tid[i] = std::thread(func, &pcp);
	}
	for(int i = 0; i < times; i++)
		tid[i].join();
	std::cout<<std::endl;

	return 0;
}

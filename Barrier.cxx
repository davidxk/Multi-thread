#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

class BarrierSync
{
public:
	BarrierSync(int n): num(n), count(0), generation(0) { }
	void sync();
private:
	std::condition_variable cv;
	std::mutex mtx;
	int num, count, generation;
};

void BarrierSync::sync()
{
	std::unique_lock<std::mutex> lck(mtx);
	if(++count < num)
	{
		int mygen = generation;
		while(mygen == generation)
			cv.wait(lck);
	}
	else
	{
		count = 0;
		generation++;
		cv.notify_all();
	}
}

int main()
{
	const int NUM = 5;
	std::thread tid[NUM];
	BarrierSync barrier(NUM);
	auto unsynced = []() { std::cout<<"----->"<<std::endl; };
	for(int i = 0; i < NUM; i++)
	{
		tid[i] = std::thread(unsynced);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
	for(int i = 0; i < NUM; i++)
		tid[i].join();
	std::cout<<std::endl;

	auto synced = [](BarrierSync& barrier) {
		barrier.sync(); std::cout<<"----->"<<std::endl;
	};
	for(int i = 0; i < NUM; i++)
	{
		tid[i] = std::thread(synced, std::ref(barrier));
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
	for(int i = 0; i < NUM; i++)
		tid[i].join();
	std::cout<<std::endl;
}

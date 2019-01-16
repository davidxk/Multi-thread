/*
 * C++11 lock manages a mutex object. Locks it upon construction, unlocks it
 * upon destruction. It also means the it unlocks the mutex upon exceptions. 
 * lock_guard cannot be unlocked. Unlock it by getting it out of scope. 
 * unique_lock can be explicitly unlocked. 
 */

#include <condition_variable>
#include <mutex>

class ReaderWriter
{
public:
	ReaderWriter(): readers(0), writers(0), active_writers(0) { }
	void reader();
	void writer();
private:
	int readers, writers;
	int active_writers;
	std::mutex mtx;
	std::condition_variable readersQ, writersQ;
};

void ReaderWriter::reader()
{
	std::unique_lock<std::mutex> lck(mtx);
	while(writers > 0)
		readersQ.wait(lck);
	readers++;
	lck.unlock();
	
	// read here
	lck.lock();
	if(--readers == 0)
		writersQ.notify_one();
	lck.unlock();
}

void ReaderWriter::writer()
{
	std::unique_lock<std::mutex> lck(mtx);
	writers++;
	while(readers > 0 or writers > 0)
		writersQ.wait(lck);
	active_writers++;
	lck.unlock();
	// write here
	lck.lock();
	active_writers--;
	if(--writers != 0)
		writersQ.notify_one();
	else
		readersQ.notify_all();
	lck.unlock();
}

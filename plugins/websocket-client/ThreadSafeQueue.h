#ifndef __EBS_THREAD_SAFE_QUEUE__
#define __EBS_THREAD_SAFE_QUEUE__

using namespace std;

#include <mutex>
#include <queue>
#include <condition_variable>

template <class T>
class ThreadSafeQueue {
public:
	~ThreadSafeQueue();
	bool push(T item);
	bool pop(T *item);
	bool awaitItem(T *item);
	void close();
private:
	std::queue<T> innerQueue;
	std::mutex queueMutex;
	std::condition_variable dataSync;
	bool dataAvailable = false;
	bool closing = false;
};

template <class T>
ThreadSafeQueue<T>::~ThreadSafeQueue()
{
	close();
}

template <class T>
bool ThreadSafeQueue<T>::push(T item)
{
	lock_guard<mutex> lock(queueMutex);

	innerQueue.push(item);
	dataAvailable = true;

	dataSync.notify_one();

	return true;
}

template <class T>
bool ThreadSafeQueue<T>::pop(T *item)
{
	lock_guard<mutex> lock(queueMutex);
	bool result = false;

	if (innerQueue.size()) {
		*item = (T)innerQueue.front();
		innerQueue.pop();
		dataAvailable = innerQueue.size() > 0;
		result = true;
	}

	return result;
}

template <class T>
bool ThreadSafeQueue<T>::awaitItem(T *item)
{
	bool retrieved = false;
	unique_lock<mutex> lock(queueMutex);

	while (!retrieved)
	{
		dataSync.wait(lock, [=] { return dataAvailable || closing; });

		lock.unlock();

		// Shutting down the queue.
		if (closing) {
			break;
		}

		retrieved = this->pop(item);
	}

	return retrieved;
}

template <class T>
void ThreadSafeQueue<T>::close()
{
	lock_guard<mutex> lock(queueMutex);
	closing = true;
	dataSync.notify_all();
}

#endif

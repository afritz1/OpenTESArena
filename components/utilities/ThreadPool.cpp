#include "ThreadPool.h"

void Worker::init(ThreadPool *pool)
{ 
    this->parentPool = pool;
    this->busy = false;
}

Worker::~Worker()
{
    this->join();
}

void Worker::invoke(std::function<void()> &&func)
{
    this->join();
    this->notifyBusy();
    this->context = std::thread([this, func]()
    {
    	func();
    	this->notifyIdle(); // It's likely the pool is waiting for an idle worker.
    });  
}

void Worker::notifyBusy()
{
    this->parentPool->signalWorkerBusy();
    this->busy = true;
}

void Worker::notifyIdle()
{
    this->busy = false;
    this->parentPool->signalWorkerIdle();
}

void Worker::join()
{
    if (this->context.joinable())
    {
        this->context.join();
    }
}

ThreadPool::ThreadPool(int threadCount)
{		
    this->workers.init(threadCount);
	for (int i = 0; i < threadCount; i++)
	{
		this->workers[i].init(this);
	}

	this->idleWorkerCount = threadCount;
}

int ThreadPool::getBusyWorkerCount()
{
	return this->workers.getCount() - this->getIdleWorkerCount();
}

int ThreadPool::getIdleWorkerCount()
{
    return this->idleWorkerCount;
}

Worker &ThreadPool::getWorker(int index)
{
    return this->workers[index];
}

void ThreadPool::signalWorkerIdle()
{
    this->idleWorkerCount.fetch_add(1);
    this->cv.notify_all();
}

void ThreadPool::signalWorkerBusy()
{
    this->idleWorkerCount.fetch_sub(1);
}

int ThreadPool::nextWorkerIndexBlocking()
{
    std::unique_lock<std::mutex> lock(this->mtx);
	this->cv.wait(lock, [this]() { return this->getIdleWorkerCount() > 0; });
	
    const auto idleIter = std::find_if(this->workers.begin(), this->workers.end(),
		[](const Worker &worker)
	{
		return !worker.busy;
	});
	
    return static_cast<int>(std::distance(this->workers.begin(), idleIter));
}
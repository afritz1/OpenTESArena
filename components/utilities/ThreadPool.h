#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "Buffer.h"
#include <future>
#include <thread>

class ThreadPool;

// A thin wrapper around std::thread to be used in the thread pool.
class Worker
{
private:
	std::jthread context;
	std::mutex mtx;
    ThreadPool *parentPool;
public:
	std::atomic<bool> busy;

	void init(ThreadPool *pool);

	// Do the thing.
	void invoke(std::function<void()> &&func);

	// Signal to the pool that we're busy.
	void notifyBusy();

	// Signal to the pool that we're idle.
	void notifyIdle();

	void join();
};

class ThreadPool
{
private:
	Buffer<Worker> workers;
	std::mutex mtx;
	std::condition_variable cv;
	std::atomic<int> idleWorkerCount;
public:
	ThreadPool(int threadCount);

	int getBusyWorkerCount();

	int getIdleWorkerCount();

	Worker &getWorker(int index);

    void signalWorkerIdle();
    void signalWorkerBusy();

	// Waits for an idle worker to become available then returns its index.
	int nextWorkerIndexBlocking();
};

#endif
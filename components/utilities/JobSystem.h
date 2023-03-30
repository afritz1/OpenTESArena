#ifndef JOB_SYSTEM_H
#define JOB_SYSTEM_H

#include <algorithm>
#include <atomic>
#include <deque>
#include <functional>
#include <thread>

#include "Buffer.h"
#include "BufferView.h"

using Job = std::function<void()>;

class JobQueue
{
private:
	std::deque<Job> jobs;
	std::condition_variable cv;
	std::mutex mtx;
public:
	[[nodiscard]] bool isEmpty()
	{
		std::lock_guard<std::mutex> lock(this->mtx);
		return this->jobs.empty();
	}

	void push(Job &&job)
	{
		std::unique_lock<std::mutex> lock(this->mtx);
		this->jobs.emplace_back(std::move(job));
		lock.unlock();

		this->cv.notify_one();
	}

	[[nodiscard]] Job pop()
	{
		std::unique_lock<std::mutex> lock(this->mtx);
		while (this->jobs.empty())
		{
			this->cv.wait(lock);
		}

		Job first = std::move(this->jobs.front());
		this->jobs.pop_front();
		return first;
	}
};

// A thin wrapper around std::thread to be used in the thread pool.
class Worker
{
private:
	std::thread context;
	std::mutex mtx;
	// Careful: here be dragons.
	std::condition_variable *poolIdleNotifierCV; // Ping this to tell the pool we're idle.
	std::atomic<int> *poolIdleCount; // To allow the pool to easily check if it has idle workers.
public:
	std::atomic<bool> busy;

	void init(std::condition_variable *idleNotifierCV, std::atomic<int> *idleCount)
	{
		this->poolIdleNotifierCV = idleNotifierCV;
		this->poolIdleCount = idleCount;
		this->busy = false;
	}

	~Worker()
	{
		this->join();
	}

	// Do the thing.
	void invoke(std::function<void()> &&func)
	{
		this->join();
		this->notifyBusy();
		this->context = std::thread([this, func]()
		{
			func();
			this->notifyIdle(); // It's likely the pool is waiting for an idle worker.
		});
	}

	// Signal to the pool that we're busy.
	void notifyBusy()
	{
		this->poolIdleCount->fetch_sub(1);
		this->busy = true;
	}

	// Signal to the pool that we're idle.
	void notifyIdle()
	{
		this->busy = false;
		this->poolIdleCount->fetch_add(1);
		this->poolIdleNotifierCV->notify_all();
	}

	void join()
	{
		if (this->context.joinable())
		{
			this->context.join();
		}
	}
};

class ThreadPool
{
private:
	Buffer<Worker> workers;
	std::mutex mtx;
	std::condition_variable cv;
	std::atomic<int> idleWorkerCount;
public:
	ThreadPool(int threadCount)
	{
		this->workers.init(threadCount);
		for (int i = 0; i < threadCount; i++)
		{
			this->workers[i].init(&this->cv, &this->idleWorkerCount);
		}

		this->idleWorkerCount = threadCount;
	}

	int getBusyWorkerCount()
	{
		return this->workers.getCount() - this->getIdleWorkerCount();
	}

	int getIdleWorkerCount()
	{
		return this->idleWorkerCount;
	}

	Worker &getWorker(int index)
	{
		return this->workers[index];
	}

	// Waits for an idle worker to become available then returns its index.
	// TODO: Optimize this by keeping a cache of idle workers.
	int nextWorkerIndexBlocking()
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
};

// TODO: This class was written before Worker was, so a lot of Worker logic is
// needlessly backported and reimplemented here (except, of course, this one
// exposes all the ugly std::thread wiring underneath.) Fixing that, by changing
// this->context to be a Worker, would be cool.
class JobManager
{
private:
	JobQueue jobQueue;
	std::unique_ptr<ThreadPool> pool;
	std::thread context;
	std::mutex mtx;
	std::condition_variable cv;
	std::atomic<bool> running = false;
public:
	void init(int threadCount)
	{
		this->pool = std::make_unique<ThreadPool>(threadCount);
	}

	~JobManager()
	{
		if (this->context.joinable())
		{
			this->context.join();
		}
	}

	bool isRunning()
	{
		return this->running;
	}

	// Adds new jobs to the queue, and if the job system is not running
	// (most likely because it's already gone through all the jobs in the queue)
	// it kicks things off again.
	void submitJobs(BufferView<Job> jobs)
	{
		for (Job &job : jobs)
		{
			this->jobQueue.push(std::move(job));
		}

		this->run();
	}

	void submitJob(Job &job)
	{
		this->submitJobs(BufferView<Job>(&job, 1));
	}

	// Waits the calling thread until it's notified by the job system that there
	// are no more jobs in the queue.
	void wait()
	{
		std::unique_lock<std::mutex> lock(this->mtx);
		this->cv.wait(lock, [this]() { return !this->isRunning(); });
	}
private:
	//  Only call this directly if you know what you're doing.
	void run()
	{
		if (this->isRunning())
		{
			return;
		}

		if (this->context.joinable())
		{
			this->context.join();
		}

		auto distributeJobs = [this]()
		{
			this->running = true;
			while (!this->jobQueue.isEmpty())
			{
				const int nextWorkerIndex = this->pool->nextWorkerIndexBlocking();
				Worker &worker = this->pool->getWorker(nextWorkerIndex);
				worker.invoke(this->jobQueue.pop());
			}

			this->running = false;
			this->cv.notify_all(); // If anyone's been waiting for us to get done with it.
		};

		this->context = std::thread(distributeJobs);
	}
};

#endif

#ifndef JOB_SYSTEM_H
#define JOB_SYSTEM_H

#include <deque>
#include <functional>
#include <thread>
#include <future>

#include "Buffer.h"
#include "BufferView.h"
#include "ThreadPool.h"
#include "ThreadSafeQueue.h"

using Job = std::function<void()>;

class JobManager
{
private:
	ThreadSafeQueue<Job> jobQueue;
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
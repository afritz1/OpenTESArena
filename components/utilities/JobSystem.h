#ifndef JOB_SYSTEM_H
#define JOB_SYSTEM_H

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <future>

#include "Buffer.h"
#include "BufferView.h"

#include "ThreadPool.h"

#include "ThreadSafeQueue.h"
#include "ThreadSafeMap.h"

using Category = std::string;

struct Job
{
	std::function<void()> task;
	Category category;

	Job(std::function<void()> task, Category category = "") 
	{ 
		this->task = task; 
		this->category = category; 
	}
};

using JobQueue = ThreadSafeQueue<Job>;

class JobManager
{
private:
	ThreadSafeMap<Category, JobQueue> queues;
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

	// Adds new jobs to the queues, and if the job system is not running
	// (most likely because it's already gone through all the jobs in the queue)
	// it kicks things off again.
	void submitJobs(BufferView<Job> jobs)
	{
		for (Job &job : jobs)
		{
			JobQueue &into = this->queues[job.category];
			into.push(std::move(job));
		}

		this->run();
	}

	void submitJob(Job &job)
	{
		this->submitJobs(BufferView<Job>(&job, 1));
	}

	// Waits the calling thread until it's notified by the job system that there
	// are no more jobs in the queues.
	void wait()
	{
		std::unique_lock<std::mutex> lock(this->mtx);
		this->cv.wait(lock, [this] { return !this->isRunning(); });
	}

	void wait(Category category)
	{
		std::unique_lock<std::mutex> lock(this->mtx);

		this->queues[category].EmptyCV.wait(lock, 
			[this, &category] 
			{ 
				return !this->isRunning() && this->queues[category].isEmpty();
			}
		);
	}

	[[nodiscard]] bool allQueuesEmpty()
	{
		std::lock_guard<std::mutex> lock(this->mtx);
		bool result = true;
		this->queues.forEach([&result](auto &entry){
			if(!entry.second.isEmpty()) {
				result = false;
				return;
			}
		});
		return result;
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
			while (!this->allQueuesEmpty())
			{
				this->queues.forEach([this](auto &entry) 
				{
					JobQueue *queue = &entry.second;
					if(!queue->isEmpty())
						return
					auto index = this->pool->nextWorkerIndexBlocking();
					auto worker = &this->pool->getWorker(index);
					worker->invoke( std::move(queue->pop().task) );
				});
			}
			this->running = false;
			this->cv.notify_all(); // IfIf anyone's been waiting for us to get done with it.
		};

		this->context = std::thread(distributeJobs);
	}
};

#endif
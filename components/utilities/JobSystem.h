#ifndef JOB_SYSTEM_H
#define JOB_SYSTEM_H

#include <algorithm>
#include <atomic>
#include <deque>
#include <functional>
#include <future>
#include <thread>
#include <vector>

using Job = std::function<void()>;

class JobQueue
{
private:
	std::condition_variable cv;
	std::mutex mt;
	std::deque<Job> jobs;
public:
	void push(Job &job)
	{
		std::unique_lock<std::mutex> lock(this->mt);
		this->jobs.emplace_back(job);
		lock.unlock();

		this->cv.notify_one();
	}

	[[nodiscard]] bool empty()
	{
		std::lock_guard<std::mutex> lock(this->mt);
		return this->jobs.empty();
	}

	[[nodiscard]] int size()
	{
		std::lock_guard<std::mutex> lock(this->mt);
		return static_cast<int>(this->jobs.size());
	}

	[[nodiscard]] Job pop()
	{
		std::unique_lock<std::mutex> lock(this->mt);
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
	std::condition_variable *pool__idle_notifier; // Ping this to tell the pool we're idle.
	std::atomic<int> *pool__idle_counter; // To allow the pool to easily check if it has idle workers.
public:
	std::atomic<bool> busy = false;

	Worker(const Worker&) = delete;

	Worker(std::condition_variable *idle_notifier, std::atomic<int> *idle_counter)
		: pool__idle_notifier(idle_notifier), pool__idle_counter(idle_counter)
	{
	}

	Worker(Worker &&worker)
		: pool__idle_notifier(std::move(worker.pool__idle_notifier)), pool__idle_counter(std::move(worker.pool__idle_counter))
	{
	}

	// Do the thing.
	void invoke(std::function<void()> &&what)
	{
		this->join();
		this->notifyBusy();
		this->context = std::thread(([this, what]()
		{
			what();
			this->notifyIdle(); // It's likely the pool is waiting for an idle worker.
		}));
	}

	// Signal to the pool that we're busy.
	void notifyBusy()
	{
		this->pool__idle_counter->fetch_sub(1);
		this->busy = true;
	}

	// Signal to the pool that we're idle.
	void notifyIdle()
	{
		this->busy = false;
		this->pool__idle_counter->fetch_add(1);
		this->pool__idle_notifier->notify_all();
	}

	void join()
	{
		if (this->context.joinable())
			this->context.join();
	}

	~Worker()
	{
		this->join();
	}
};

class ThreadPool
{
private:
	std::mutex mt;
	std::condition_variable cv;
	std::vector<Worker> workers;
	std::atomic<int> idle_workers;
public:
	// Returns an iterator (NOT a pointer) to the first idle worker it finds in
	// the pool. If there isn't one, it waits.
	// TODO: Optimize this by keeping a cache of idle workers.
	auto requestIdleWorker()
	{
		std::unique_lock<std::mutex> lock(this->mt);
		this->cv.wait(lock, [this] { return this->idleWorkers() > 0; });

		return std::find_if(workers.begin(), workers.end(), [](Worker &worker)
		{
			return !(worker.busy.load());
		});
	}

	int busyWorkers()
	{
		return static_cast<int>(this->workers.size() - this->idleWorkers());
	}

	int idleWorkers()
	{
		return this->idle_workers.load();
	}

	ThreadPool(int n_workers)
	{
		for (int n = 0; n < n_workers; n++)
			workers.emplace_back(&this->cv, &this->idle_workers);
		this->idle_workers.store(n_workers);
	}
};

// TODO: This class was written before Worker was, so a lot of Worker logic is
// needlessly backported and reimplemented here (except, of course, this one
// exposes all the ugly std::thread wiring underneath.) Fixing that, by changing
// this->context to be a Worker, would be cool.
class JobManager
{
private:
	std::mutex mtx;
	std::condition_variable cv;
	std::unique_ptr<ThreadPool> pool;
	std::thread context;
	std::atomic<bool> running = false;
	JobQueue job_queue;
public:
	// Adds new jobs to the queue, and if the job system is not running
	// (most likely because it's already gone through all the jobs in the queue)
	// it kicks things off again.
	void submitJobs(std::vector<Job> jobs)
	{
		for (Job &new_job : jobs)
		{
			this->job_queue.push(new_job);
		}

		this->run();
	}

	// Waits the calling thread until it's notified by the job system that there
	// are no more jobs in the queue.
	void wait()
	{
		std::unique_lock l(this->mtx);
		this->cv.wait(l, [this] { return !this->isRunning(); });
	}

	bool isRunning()
	{
		return this->running.load();
	}

	JobManager(int n_threads)
	{
		this->pool = std::make_unique<ThreadPool>(n_threads);
	}

	JobManager(int n_threads, std::vector<Job> &jobs)
	{
		this->pool = std::make_unique<ThreadPool>(n_threads);
		this->submitJobs(jobs);
	}

	~JobManager()
	{
		if (this->context.joinable())
			this->context.join();
	}
private:
	//  Only call this directly if you know what you're doing.
	void run()
	{
		if (this->isRunning())
			return;

		if (this->context.joinable())
			this->context.join();

		auto distribute_jobs_across_workers = [this]()
		{
			this->running = true;
			while (!this->job_queue.empty())
			{
				auto worker = this->pool->requestIdleWorker();
				worker->invoke(this->job_queue.pop());
			}

			this->running = false;
			this->cv.notify_all(); // If anyone's been waiting for us to get done with it.
		};

		this->context = std::thread(distribute_jobs_across_workers);
	}
};

#endif

/*  
    |> The engine is lacking a good way of utilizing several threads. 
    |> After the software rendering is optimized for single-threaded execution in part 3, 
    |> it will switch to this job system so pixel shading, etc. can run 5-20x faster.
    |> 
    |> Off the top of my head, the job system needs a private thread pool and some kind of addJob(), 
    |> isRunning(), and wait() functions. Maybe it could have a queue of std::function<void()> 
    |> and worker threads would check if the queue is not empty. Later, wait() could take a 
    |> category tag so it only waits for certain jobs to finish. Basically the job system exists 
    |> so we don't have to use condition variables directly.
    
    - https://github.com/afritz1/OpenTESArena/issues/245

*/

#include <thread>
#include <atomic>
#include <future>

#include <functional>
#include <deque>
#include <vector>
#include <algorithm>

#include <iostream>

namespace JobSystem
{   
    class Job 
    {   
    public:
        auto operator()() { this->task(); }   
        
        Job(std::function<void()> &&task)
            : task(std::move(task)) 
        {}
    private:
        std::function<void()> task;
    };

    class JobQueue 
    {
    public:
        // Add a job to the back of the queue.
    	void enqueue(Job &job) 
        {
    		{
    			std::unique_lock<std::mutex> lock(this->mt);
    			this->jobs.push_back(job);
    		}
    		this->cv.notify_one();
    	}

        [[nodiscard]] bool empty()
        {
            std::unique_lock<std::mutex> lock(this->mt);
            return this->jobs.empty();
        }

        [[nodiscard]] std::size_t size()
        {
            std::unique_lock<std::mutex> lock(this->mt);
            return this->jobs.size();
        }

        // Pop a job from the front of the queue.
    	[[nodiscard]] Job pop_front() 
        {
    		std::unique_lock<std::mutex> lock(this->mt);
    		while (this->jobs.empty())
    			this->cv.wait(lock);
    		Job first = std::move(this->jobs.front());
    		this->jobs.pop_front();
    		return first;
    	}

    private:
        std::condition_variable cv;
    	std::mutex mt;
    	std::deque<Job> jobs;
    };

    /// @brief A thin wrapper around std::thread to be used in the thread pool.
    class Worker
    {
    private:
        std::thread context;   
        std::mutex mtx;
        // Careful: here be dragons.
        std::condition_variable    *pool__idle_notifier; // Ping this to tell the pool we're idle.
        std::atomic<std::uint32_t> *pool__idle_counter;  // To allow the pool to easily check if it has idle workers.

    public:
        std::atomic<bool> busy = false;

        Worker(const Worker&) = delete;

        Worker(std::condition_variable *idle_notifier, std::atomic<std::uint32_t> *idle_counter)
            : pool__idle_notifier(idle_notifier),
              pool__idle_counter (idle_counter) 
        { }

        Worker(Worker &&worker)
            : pool__idle_notifier(std::move(worker.pool__idle_notifier)),
              pool__idle_counter (std::move(worker.pool__idle_counter))
        { }

        // Do the thing.
        void invoke(std::function<void()> &&what) 
        {       
            this->join();
            this->notifyBusy();
            this->context = std::thread ( ([this, what] () { 
                what();
                this->notifyIdle();  // It's likely the pool is waiting for an idle worker.
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

        void join() { 
            if(this->context.joinable()) 
                this->context.join(); 
        }

        ~Worker() { this->join(); }
    };

    class ThreadPool
    {   
    public:

        // Returns an iterator (NOT a pointer) to the first idle worker it finds in the pool.
        // If there isn't one, it waits.
        // TODO: Optimize this by keeping a cache of idle workers.
        auto requestIdleWorker()
        {       
            std::unique_lock<std::mutex> lock(this->mt);
            this->cv.wait(lock, [this]{ return this->idleWorkers() > 0; });

            return std::find_if(workers.begin(), workers.end(), [](Worker &worker) { 
                    return !(worker.busy.load()); 
                }
            );
        }

        std::uint32_t busyWorkers() 
        {
            return this->workers.size() - this->idleWorkers();
        }

        std::uint32_t idleWorkers()
        {
            return this->idle_workers.load();
        }

        ThreadPool(std::size_t n_workers) 
        { 
            for(std::size_t n = 0 ; n < n_workers ; n++) 
                workers.emplace_back(&this->cv, &this->idle_workers);
            this->idle_workers.store(n_workers);
        }

    private: 
        std::mutex mt;
        std::condition_variable cv;

        std::vector< Worker > workers;
        std::atomic<std::uint32_t> idle_workers; 
    };
    
    // TODO: This class was written before Worker was, so a lot of Worker logic is 
    // needlessly backported and reimplemented here (except, of course, this one exposes all the ugly std::thread wiring underneath.)
    // Fixing that, by changing this->context to be a Worker, would be cool.
    class JobManager
    {
    public:

        // Adds new jobs to the queue, and if the job system is not running 
        // (most likely because it's already gone through all the jobs in the queue) it kicks things off again.
        void submitJobs(std::vector<Job> jobs) 
        {       
            for(auto &new_job: jobs)
                this->job_queue.enqueue(new_job);

            this->run();
        }   

        // Waits the calling thread until it's notified by the job system that there are no more jobs in the queue.
        void wait()
        {
            std::unique_lock l(this->mtx);
            this->cv.wait(l, [this]{ return !this->isRunning(); } );
        }

        bool isRunning() 
        { return this->running.load(); }

        JobManager(std::size_t n_threads) 
        {
            this->pool = std::make_unique<ThreadPool>(n_threads);
        } 
        
        JobManager(std::size_t n_threads, std::vector<Job> &jobs) 
        {
            this->pool = std::make_unique<ThreadPool>(n_threads);
            this->submitJobs(jobs);
        } 
        
        ~JobManager() 
        { 
            if(this->context.joinable()) 
                this->context.join(); 
        }

    private:
        
        //  Only call this directly if you know what you're doing.
        void run()
        {   
            if(this->isRunning())
                return; 

            if(this->context.joinable())
                this->context.join();

            auto distribute_jobs_across_workers = [this]() {
                this->running = true;
                while(!this->job_queue.empty())  
                { 
                    auto worker = this->pool->requestIdleWorker();
                    worker->invoke(this->job_queue.pop_front());
                } 
                this->running = false;
                this->cv.notify_all(); // If anyone's been waiting for us to get done with it.
            };

            this->context = std::thread(distribute_jobs_across_workers);
        }

        std::mutex mtx;
        std::condition_variable cv;
        std::unique_ptr<ThreadPool> pool;
        std::thread context;

        std::atomic<bool> running = false;

        JobQueue job_queue;
    };  
}
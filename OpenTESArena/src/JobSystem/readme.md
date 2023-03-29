# OpenJobArena (oJa)

This implements a simple and safe job system.

## Examples:

```c++
// If the job manager has at least 3 threads to work with, this 
// only takes 1 second.
{
    auto jm = JobManager(3);
    
    Job sleep ([](){ 
       std::this_thread::sleep_for(1000ms);
    });

    jm.submitJobs ( { sleep, sleep, sleep } );
}

// This, too, should only take 1 second.
{
    auto max_threads = std::thread::hardware_concurrency(); 
    auto jm = JobManager(max_threads);
    
    std::vector< Job > job_list (
       max_threads,
       Job( [](){ std::this_thread::sleep_for(1000ms); } )
    );
     
    jm.submitJobs ( job_list );
}

// This will wait for all jobs to be done before resuming the calling thread.
{
    auto max_threads = std::thread::hardware_concurrency(); 
    auto jm = JobManager(max_threads);
    
    Job sleep ([](){ 
       std::this_thread::sleep_for(1000ms);
    });
    
    jm.submitJobs( { sleep } );

    jm.wait();
    std::cout << "\nDone!";
}
``` 

Essentially, when a ``JobManager`` is constructed with an ``n_threads`` argument, it creates a private ``ThreadPool``.
That ``ThreadPool``, in turn, creates n_threads ``Workers``. 

``JobManager::run()`` spawns a background thread to do the following:
1) While the job queue is not empty, ask the thread pool for an idle worker. 
   
    If the thread pool can't immediately provide an idle worker, the background thread will wait until one is available. 

2) When an idle worker has been obtained, pop a job from the front of the queue and tell the idle worker to do it.





``JobManager``, if given a list of jobs in its constructor (optional), immediately starts running the job distributor.
Otherwise, ``JobManager::run()`` only runs when the following conditions are met:
1) ``JobManager::submitJobs()`` has been called.

2) ``JobManager::isRunning()`` returns false.

3) The job queue is not empty.

This means the only job-performing method you need to worry about is ``JobManager::submitJobs()``, and that adding jobs to an already-running queue is completely safe and intended behaviour: 
```c++
{
    auto jm = JobManager(std::thread::hardware_concurrency());
    
    Job sleep ([](){ 
       std::this_thread::sleep_for(1h);
    });

    // Since the queue was empty before, this starts the job system, 
    // which will run for an hour.
    jm.submitJobs ( { sleep } ); 

    // Completely safe, and a new instance of sleep has been appended to
    // jm's already-existing queue.
    jm.submitJobs ( { sleep } ); 
}
```


## To do: 
    
    [ ] Categories (filter which jobs JobManager::wait() waits for)
    [ ] Decentralize job distribution a bit more.

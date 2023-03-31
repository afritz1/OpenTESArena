#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

template<typename T>
class ThreadSafeQueue
{
private:
	std::deque<T> items;
	std::condition_variable cv;
	std::mutex mtx;
public:
	[[nodiscard]] bool isEmpty()
	{
		std::lock_guard<std::mutex> lock(this->mtx);
		return this->items.empty();
	}

	void push(T &&item)
	{
		std::unique_lock<std::mutex> lock(this->mtx);
		this->items.emplace_back(std::move(item));
		lock.unlock();

		this->cv.notify_one();
	}

	[[nodiscard]] T pop()
	{
		std::unique_lock<std::mutex> lock(this->mtx);
		while (this->items.empty())
		{
			this->cv.wait(lock);
		}
        
		T first = std::move(this->items.front());
		this->items.pop_front();
		return first;
	}

	[[nodiscard]] std::size_t getSize()
    {
        std::lock_guard<std::mutex> lock(this->mtx);
        return this->items.size();
    }
};

#endif
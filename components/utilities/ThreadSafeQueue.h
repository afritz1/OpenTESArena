#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

template<typename T>
class ThreadSafeQueue
{
private:
	std::deque<T> items;
	std::mutex mtx;
public:
	std::condition_variable EmptyCV;  
	std::condition_variable notEmptyCV;
	[[nodiscard]] bool isEmpty()
	{
		std::lock_guard<std::mutex> lock(this->mtx);
		auto empty = this->items.empty();
		if(empty)
			this->EmptyCV.notify_all();
		return empty;
	}

	void push(T &&item)
	{
		std::unique_lock<std::mutex> lock(this->mtx);
		this->items.emplace_back(std::move(item));
		lock.unlock();

		this->notEmptyCV.notify_all();
	}

	[[nodiscard]] T pop()
	{
		std::unique_lock<std::mutex> lock(this->mtx);
		
		this->notEmptyCV.wait(lock, [this]{ return !this->items.empty(); });
        
		T first = std::move(this->items.front());
		this->items.pop_front();

		if(this->items.empty())
			this->EmptyCV.notify_all();
		
		return first;
	}

	[[nodiscard]] std::size_t getSize()
    {
        std::lock_guard<std::mutex> lock(this->mtx);
        return this->items.size();
    }
};

#endif
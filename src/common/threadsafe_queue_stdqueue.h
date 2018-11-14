#ifndef threadsafe_queue_h__
#define threadsafe_queue_h__

#include <queue>
#include <mutex>

template<typename T>
class threadsafe_queue
{
private:
	std::queue<T> _queue;
	mutable std::mutex _mutex;
	std::condition_variable _condition_variable;

public:
	void push(T const& data)
	{
		std::unique_lock<std::mutex> lock(_mutex);
		_queue.push(data);
		lock.unlock();
		_condition_variable.notify_one();
	}

	bool empty() const
	{
		std::lock_guard<std::mutex> lock(_mutex);
		return _queue.empty();
	}

	bool try_pop(T& popped_value)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_queue.empty())
		{
			return false;
		}

		popped_value = _queue.front();
		_queue.pop();
		return true;
	}

	void wait_and_pop(T& popped_value)
	{
		std::unique_lock<std::mutex> lock(_mutex);
		while (_queue.empty())
		{
			_condition_variable.wait(lock);
		}

		popped_value = _queue.front();
		_queue.pop();
	}

};

#endif // threadsafe_queue_h__

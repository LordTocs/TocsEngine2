#pragma once
#include <atomic>
#include <vector>

namespace tocs {
namespace threading {
namespace detail {

//https://gist.github.com/jcdickinson/ec2b93f78afc4c72ae74

template <class T>
class work_queue
{
	std::size_t max_work;
	std::unique_ptr<std::atomic<T>[]> work_array;
	std::atomic<int> top;
	std::atomic<int> bottom;

public:

	static_assert(std::atomic<T>::is_always_lock_free, "work queues only work on lock free types");

	work_queue(std::size_t max_work)
		: max_work(max_work)
		, work_array(new std::atomic<T> [max_work])
	{
	}

	std::size_t get_max_work() const { return max_work; }

	bool push(const T &item)
	{
		int b = bottom;
		work_array[b] = item;
		bottom = b + 1;
	}

	bool pop(T &result)
	{
		int b = bottom - 1;
		bottom = b;

		int t = top;
		if (t <= b)
		{

			result = work_array[b];
			if (t != b)
			{
				return true;
			}

			bool success = true;
			if (top.compare_exchange_strong(t, t + 1))
			{
				success = false;
			}

			bottom = t + 1;
			return success;
		}
		else
		{
			bottom = t;
			return false;
		}
	}

	bool steal(T &result)
	{
		int t = top;
		int b = bottom;
		if (t < b)
		{
			result = work_array[t];
			if (top.compare_exchange_strong(t, t+1))
			{
				return false;
			}
			return true;
		}
		return false;
	}
};

}
}
}

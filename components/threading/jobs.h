#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include "cacheline.h"
#include "pool.h"

namespace tocs {
namespace threading {

//https://manu343726.github.io/2017/03/13/lock-free-job-stealing-task-system-with-modern-c.html

class job
{
	job *parent;
	std::atomic<int> remaining_subjobs;
	std::function<void()> job_func;

	concurrent_pool_handle<job> pool_handle;

	cache_line_padding padding;
public:

	job()
		: parent(nullptr)
	{}

	template<class Func>
	job(Func &&func)
		: parent(nullptr)
		, job_func(std::forward<Func>(func))
	{}

	void run();

	static job *this_job();
};


}
}
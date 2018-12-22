#pragma once
#include "workqueue.h"
#include "jobs.h"
#include "pool.h"
#include <random>

namespace tocs {
namespace threading {

class job_system;

class worker
{
	detail::work_queue<job*> job_queue;
	job_system *system;
	std::mt19937 worker_chooser;

	cache_line_padding padding;

	job *pull_job();
public:
	worker(job_system &system, int maxwork)
		: system(&system)
		, job_queue(maxwork)
	{
	}


	void run();

	static worker* this_worker();

	template <class Func>
	pool_ptr<job> queue_job(Func &&func)
	{
		pool_ptr<job> new_job = system->job_pool.get_item(std::forward<Func>(func));
		job_queue.push(new_job);
		return new_job;
	}
};

class job_system
{
	std::vector<worker> workers;
	std::uniform_int_distribution<int> worker_choosing_range;
	std::vector<std::thread> threads;
	concurrent_pool<job> job_pool;
public:
	friend class worker;

	job_system(std::size_t worker_count = std::thread::hardware_concurrency())
		: workers(worker_count)
		, worker_choosing_range(0, worker_count)
	{
		threads.reserve(workers.size() - 1);
		for (int i = 0; i < threads.size(); ++i)
		{
			threads.emplace_back([worker=&workers[i], i]() 
			{
				worker->run();
			});
		}
	}

};

}
}
#pragma once
#include "workqueue.h"
#include "jobs.h"
#include "pool.h"
#include <random>
#include <type_traits>

namespace tocs {
namespace threading {

class job_system;

class worker
{
	detail::work_queue<job*> job_queue;
	job_system *system;
	std::mt19937 worker_chooser;
	bool running;
	cache_line_padding padding;

	job *pull_job();
public:
	worker(job_system &system, int maxwork)
		: system(&system)
		, job_queue(maxwork)
		, running(true)
	{
	}

	~worker();

	worker(const worker &copyme) = delete;
	worker &operator=(const worker &copyme) = delete;
	
	worker(worker &&moveme) = default;
	worker &operator=(worker &&moveme) = default;

	void run();
	void stop_work() { running = false; }

	static worker* this_worker();

	template <class Func>
	threading::concurrent_pool_handle<job> queue_job(Func &&func);
};

static_assert(std::is_move_constructible<worker>::value, "Workers have to be move constructable");
static_assert(std::is_move_assignable<worker>::value, "Workers have to be move assignable");


class job_system
{
	std::vector<worker> workers;
	std::uniform_int_distribution<int> worker_choosing_range;
	std::vector<std::thread> threads;
	concurrent_pool<job> job_pool;
public:
	friend class worker;

	job_system(std::size_t worker_count = std::thread::hardware_concurrency());
	~job_system();

	job_system(const job_system &) = default;
	job_system &operator=(const job_system &) = default;

	job_system(job_system &&) = default;
	job_system &operator=(job_system &&) = default;

	template <class Func>
	static threading::concurrent_pool_handle<job> queue_job(Func &&func)
	{
		return worker::this_worker()->queue_job(std::forward<Func>(func));
	}
};

template <class Func>
threading::concurrent_pool_handle<job> worker::queue_job(Func &&func)
{
	auto new_job = system->job_pool.get_item(std::forward<Func>(func));
	job_queue.push(new_job);
	return new_job;
}

}
}
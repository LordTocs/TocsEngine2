#include "worker.h"
#include <thread>

namespace tocs {
namespace threading {

worker::~worker()
{
	job *junk_job = nullptr;
	while (job_queue.pop(junk_job))
	{ }
}

job *worker::pull_job()
{
	job *job_to_run = nullptr;
	if (!job_queue.pop(job_to_run))
	{
		int worker = system->worker_choosing_range(worker_chooser);
		auto &steal_queue = system->workers[worker].job_queue;
		if (&steal_queue == &job_queue)
		{
			std::this_thread::yield();
			return nullptr;
		}

		//Attempt a steal.
		if (!steal_queue.steal(job_to_run))
		{
			std::this_thread::yield();
			return nullptr;
		}
	}

	return job_to_run;
}

static thread_local worker *thread_worker = nullptr;
worker* worker::this_worker() { return thread_worker; }

void worker::run()
{
	thread_worker = this;
	while (running)
	{
		job *j = pull_job();
		if (j)
		{
			j->run();
		}
	}
}


job_system::job_system(std::size_t worker_count /* = std::thread::hardware_concurrency() */)
	: worker_choosing_range(0, worker_count)
{
	workers.reserve(worker_count);
	threads.reserve(worker_count - 1);

	for (unsigned int i = 0; i < worker_count; ++i)
	{
		workers.emplace_back(*this, 100);
		if (i != 0)
		{
			threads.emplace_back([w = &workers[i]]()
			{
				w->run();
			});
		}
	}
}


job_system::~job_system()
{
	for (worker &w : workers)
	{
		w.stop_work();
	}

	for (std::thread &thread : threads)
	{
		thread.join();
	}
}

}
}

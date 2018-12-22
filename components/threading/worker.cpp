#include "worker.h"
#include <thread>

namespace tocs {
namespace threading {

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
	while (true)
	{
		job *j = pull_job();
		if (j)
		{
			j->run();
		}
	}
}



}
}

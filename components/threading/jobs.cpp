#include "jobs.h"

namespace tocs {
namespace threading {

static thread_local job* thread_job;

void job::run()
{
	thread_job = this;
	job_func();
	thread_job = nullptr;
	if (parent)
	{
		--parent->remaining_subjobs;
	}
}

job *job::this_job()
{
	return thread_job;
}


}
}
#include "JobGraph.h"

void JobGraph::Schedule(JobHandle::Ptr handle) noexcept
{
    std::vector<Job::Ptr> jobs;
    handle->GetJobs(jobs);
    Schedule(handle, jobs);
}

void JobGraph::Schedule(JobHandle::Ptr handle, JobHandle::Ptr parent) noexcept
{
    std::vector<Job::Ptr> jobs;
    handle->GetJobs(jobs);
    handle->Depend(parent, jobs);
    Schedule(handle, jobs);
}

void JobGraph::Schedule(JobHandle::Ptr handle, std::vector<Job::Ptr>& jobs) noexcept
{
    for (auto job : jobs)
    {
        if (! job->IsScheduled())
        {
            mScheduledJobs.emplace_back(job);
            job->NotifyScheduled();
        }
    }
}

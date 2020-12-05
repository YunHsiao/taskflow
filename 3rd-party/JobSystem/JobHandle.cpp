#include "JobHandle.h"

void JobHandle::Depend(Ptr depend, std::vector<Job::Ptr> const& jobs) noexcept
{
    // TODO 没有检查重复依赖和环形依赖
    
    std::vector<Job::Ptr> parents;
    depend->GetJobs(parents);
    
    for (auto job : jobs)
    {
        if (job->IsScheduled())
        {
            continue;
        }
        
        for (auto parent : parents)
        {
            parent->AddChild(job);
        }
    }
}

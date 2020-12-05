#pragma once

#include <vector>
#include "Job.h"
#include "JobHandle.h"

// 只能单线程访问
class JobGraph final
{
public:
    
                            JobGraph() = default;
                            ~JobGraph() = default;
                            JobGraph(JobGraph const&) = delete;
                            JobGraph(JobGraph&&) = delete;
                            JobGraph& operator=(JobGraph const&) = delete;
                            JobGraph& operator=(JobGraph&&) = delete;
    
    void                    Schedule(JobHandle::Ptr handle) noexcept;
    void                    Schedule(JobHandle::Ptr handle, JobHandle::Ptr parent) noexcept;
    
private:
    
    void                    Schedule(JobHandle::Ptr handle, std::vector<Job::Ptr>& jobs) noexcept;
    
    std::vector<Job::Ptr>   mScheduledJobs  {};
    
    friend class            JobSystem;
};

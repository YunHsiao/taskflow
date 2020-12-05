#pragma once

#include <vector>
#include <memory>
#include "Job.h"

class JobHandle
{
public:
    
    using Ptr       = std::shared_ptr<JobHandle>;
    using PtrArray  = std::vector<Ptr>;
    
                    JobHandle() = default;
    virtual         ~JobHandle() = default;
                    JobHandle(JobHandle const&) = delete;
                    JobHandle(JobHandle&&) = delete;
                    JobHandle& operator=(JobHandle const&) = delete;
                    JobHandle& operator=(JobHandle&&) = delete;
    
    virtual void    WaitForComplete() noexcept = 0;
    
protected:
    
    virtual void    GetJobs(std::vector<Job::Ptr>& jobs) const noexcept = 0;
    
    // 一个Handle(组合)可以依赖其他Handle(组合)或被其他Handle(组合)依赖
    void            Depend(Ptr depend, std::vector<Job::Ptr> const& jobs) noexcept;
    
    friend class    JobSystem;
    friend class    JobGraph;
};

template <typename Future>
class JobHandleImpl final : public JobHandle
{
public:
    
    using           JobHandle::JobHandle;
                    JobHandleImpl(Future&& future, Job::Ptr job) noexcept;
    
    virtual void    WaitForComplete() noexcept override;
    
private:
    
    virtual void    GetJobs(std::vector<Job::Ptr>& jobs) const noexcept override;
    
    Future          mFuture;
    Job::Ptr        mJob     { nullptr };
};

template <typename Future>
JobHandleImpl<Future>::JobHandleImpl(Future&& future, Job::Ptr job) noexcept
: mFuture(std::move(future))
, mJob(job)
{
}

template <typename Future>
void JobHandleImpl<Future>::WaitForComplete() noexcept
{
    mFuture.get();
}

template <typename Future>
void JobHandleImpl<Future>::GetJobs(std::vector<Job::Ptr>& jobs) const noexcept
{
    assert(mJob);
    jobs.emplace_back(mJob);
}

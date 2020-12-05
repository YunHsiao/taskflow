#include "JobSystem.h"

JobHandle::Ptr JobSystem::CombineJobs(JobHandle::PtrArray&& handles) noexcept
{
    class JobHandleCombined final : public JobHandle
    {
    public:
        
        explicit JobHandleCombined(JobHandle::PtrArray&& handles) noexcept
        : mCombinedHandles(std::move(handles))
        {
            
        }
        
        virtual void WaitForComplete() noexcept override
        {
            for (auto handle : mCombinedHandles)
            {
                handle->WaitForComplete();
            }
        }
        
    private:
        
        virtual void GetJobs(std::vector<Job::Ptr>& jobs) const noexcept override
        {
            jobs.clear();
            jobs.reserve(mCombinedHandles.size());
            
            for (auto handle : mCombinedHandles)
            {
                handle->GetJobs(jobs);
            }
        }
        
        JobHandle::PtrArray mCombinedHandles    {};
    };
    
    JobHandle::Ptr handle = std::make_shared<JobHandleCombined>(std::move(handles));
    return handle;
}

void JobSystem::Dispatch(JobGraph& graph) noexcept
{
    if (! mRunning)
    {
        return;
    }
    
    for (auto job : graph.mScheduledJobs)
    {
        if (job->IsRoot())
        {
            mThreadPool.DispatchTask([this, job]() mutable
            {
                job->Execute(mThreadPool);
            });
        }
    }
    
    graph.mScheduledJobs.clear();
}

void JobSystem::Start() noexcept
{
    if (mRunning)
    {
        return;
    }
    
    mRunning = true;
    mThreadPool.Start();
}

void JobSystem::Stop() noexcept
{
    if (! mRunning)
    {
        return;
    }
    
    mRunning = false;
    mThreadPool.Stop();
}

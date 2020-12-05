#pragma once

#include <memory>
#include <tuple>
#include "ThreadPool.h"
#include "Job.h"
#include "JobHandle.h"
#include "JobGraph.h"
#include "WorkStealer.h"

// 支持依赖关系的任务调度系统
// 每个Job可以依赖多个其他的Job 有依赖关系的Job之间串行
// 只有没有依赖的Job才会被放到线程池的任务队列中
// 支持在任意线程Dispatch

class JobSystem final
{
public:
    
                            JobSystem() = default;
                            ~JobSystem() = default;
                            JobSystem(JobSystem const&) = delete;
                            JobSystem(JobSystem&&) = delete;
                            JobSystem& operator=(JobSystem const&) = delete;
                            JobSystem& operator=(JobSystem&&) = delete;
    
    template <typename Function, typename... Args>
    static JobHandle::Ptr   CreateJob(std::string const& name, Function&& func, Args&&... args) noexcept;
    template <typename Function, typename... Args>
    static JobHandle::Ptr   CreateJobsForEachIndex(std::string const& name, uint16_t const workCount, uint16_t const workCountPerBatch, Function&& func, Args&&... args) noexcept;
    template <typename Function, typename... Args>
    static JobHandle::Ptr   CreateJobsForEachRange(std::string const& name, uint16_t const workCount, uint16_t const workCountPerBatch, Function&& func, Args&&... args) noexcept;
    
    static JobHandle::Ptr   CombineJobs(JobHandle::PtrArray&& handles) noexcept;
    void                    Dispatch(JobGraph& graph) noexcept;
    void                    Start() noexcept;
    void                    Stop() noexcept;

private:
    
    ThreadPool              mThreadPool     {};
    std::atomic<bool>       mRunning        { false };
};

template <typename Function, typename... Args>
JobHandle::Ptr JobSystem::CreateJob(std::string const& name, Function&& func, Args&&... args) noexcept
{
    // TODO JobMemoryPool
    using ReturnType = decltype(func(std::forward<Args>(args)...));
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(std::bind(std::forward<Function>(func), std::forward<Args>(args)...));
    Job::Ptr job = std::make_shared<Job>(name, [task](){ (*task)(); });
    JobHandle::Ptr handle = std::make_shared<JobHandleImpl<std::future<ReturnType>>>(task->get_future(), job);
    return handle;
}

template <class _Tp>
inline typename std::decay<_Tp>::type decay_copy(_Tp&& __t)
{
    return std::forward<_Tp>(__t);
}

template <typename Function, typename... Args>
JobHandle::Ptr JobSystem::CreateJobsForEachIndex(std::string const& name, uint16_t const workCount, uint16_t const workCountPerBatch, Function&& func, Args&&... args) noexcept
{
    assert(workCount);
    JobHandle::PtrArray jobHandles;
    
    WorkStealer::Ptr workStealer = std::make_shared<WorkStealer>();
    auto const jobCount = workStealer->Initial(workCount, workCountPerBatch);
      
    for (auto jobIndex = 0; jobIndex < jobCount; ++jobIndex)
    {
        // 每个Work都是独立的 不能影响其他Work 所以这里把参表中的引用去掉
        std::tuple<typename std::decay<Args>::type...> argsTuple(decay_copy(std::forward<Args>(args))...);
        
        jobHandles.emplace_back(CreateJob(name + "_" + std::to_string(jobIndex), [workStealer, jobIndex, &func, _argsTuple = std::move(argsTuple)]()
        {
            uint16_t start  = 0;
            uint16_t end    = 0;
            
            while (workStealer->TrySteal(start, end, jobIndex))
            {
                for (auto i = start; i < end; ++i)
                {
                    auto argsCopy = _argsTuple;
                    
                    std::apply([i, &func](Args&&... args)
                    {
                        func(i, std::forward<Args>(args)...);
                    }, argsCopy);
                }
            }
        }));
    }
 
    return JobSystem::CombineJobs(std::forward<JobHandle::PtrArray>(jobHandles));
}

template <typename Function, typename... Args>
JobHandle::Ptr JobSystem::CreateJobsForEachRange(std::string const& name, uint16_t const workCount, uint16_t const workCountPerBatch, Function&& func, Args&&... args) noexcept
{
    assert(workCount);
    JobHandle::PtrArray jobHandles;
    
    WorkStealer::Ptr workStealer = std::make_shared<WorkStealer>();
    auto const jobCount = workStealer->Initial(workCount, workCountPerBatch);
      
    for (auto jobIndex = 0; jobIndex < jobCount; ++jobIndex)
    {
        // 每个Work都是独立的 不能影响其他Work 所以这里把参表中的引用去掉
        std::tuple<typename std::decay<Args>::type...> argsTuple(decay_copy(std::forward<Args>(args))...);
        
        jobHandles.emplace_back(CreateJob(name + "_" + std::to_string(jobIndex), [workStealer, jobIndex, &func, _argsTuple = std::move(argsTuple)]()
        {
            uint16_t start  = 0;
            uint16_t end    = 0;
            
            while (workStealer->TrySteal(start, end, jobIndex))
            {
                auto argsCopy = _argsTuple;
                
                std::apply([start, end, &func](Args&&... args)
                {
                    func(start, end, std::forward<Args>(args)...);
                }, argsCopy);
            }
        }));
    }
 
    return JobSystem::CombineJobs(std::forward<JobHandle::PtrArray>(jobHandles));
}

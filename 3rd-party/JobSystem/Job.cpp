#include "Job.h"

Job::Job(std::string const& name, ThreadPool::Task&& task) noexcept
: mTask(std::move(task))
, mName(name)
{
}

void Job::Execute(ThreadPool& threadPool) noexcept
{
    assert(mScheduled);
    mTask();
    
    auto it = mChildren.begin();
    Job::Ptr firstReadyChild = nullptr;
    
    while (it != mChildren.end())
    {
        auto child = *it++;
        
        if (child->NotifyParentFinished())
        {
            firstReadyChild = child;
            break;
        }
    }
    
    while (it != mChildren.end())
    {
        auto child = *it++;
        
        if (child->NotifyParentFinished())
        {
            threadPool.DispatchTask([child, &threadPool]() mutable
            {
                child->Execute(threadPool);
            });
        }
    }
    
    // 找一个准备好的子任务在父任务的线程执行而不是派发到线程池去等待调度
    // 这样做有两个好处：
    // 1、减少一次调度本身带来的开销
    // 2、父子之间对Cache的访问大概率会存在时间局部性 比如父写子读这种用法 在同一个线程执行可以保证利用同一个CPU核心的L1 避免在另一个核心重新读取数据到L1
    if (firstReadyChild)
    {
        firstReadyChild->Execute(threadPool);
    }
}

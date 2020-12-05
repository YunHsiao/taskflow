#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <string>
#include "ThreadSafeCounter.h"
#include "ThreadPool.h"

// 这不是给用户使用的类 用户应该用JobHandle
class alignas(64) Job final
{
public:
    
    using Ptr                   = std::shared_ptr<Job>;
    
                                Job(std::string const& name, ThreadPool::Task&& task) noexcept;
                                ~Job() = default;
                                Job(Job const&) = delete;
                                Job(Job&&) = delete;
                                Job& operator=(Job const&) = delete;
                                Job& operator=(Job&&) = delete;
    
    virtual void                Execute(ThreadPool& threadPool) noexcept;
    inline bool                 IsRoot() const noexcept             { assert(mScheduled); return mParentCount == 0; }
    inline bool                 IsScheduled() const noexcept        { return mScheduled; }
    inline void                 NotifyScheduled() noexcept          { assert(! mScheduled); mScheduled = true; }
    inline void                 AddChild(Job::Ptr child) noexcept   { ++child->mParentCount; mChildren.emplace_back(child); }
    inline bool                 NotifyParentFinished() noexcept     { assert(mScheduled); return mFinishedParentCount.Increment() == mParentCount - 1; }
    inline std::string const&   GetName() const noexcept            { return mName; }
    
private:
    
    ThreadSafeCounter<int16_t>  mFinishedParentCount    {};
    int16_t                     mParentCount            { 0 };
    bool                        mScheduled              { false };
    ThreadPool::Task            mTask                   { nullptr };
    // 上面的数据属于同一缓存行 注意只有mFinishedParentCount可以被多线程修改 否则会造成伪共享
    
    std::string                 mName                   { "" };
    std::vector<Job::Ptr>       mChildren               {};
};

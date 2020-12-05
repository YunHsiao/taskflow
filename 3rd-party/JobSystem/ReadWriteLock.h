#pragma once

#include <shared_mutex>

class ReadWriteLock final
{
public:
    
    template <typename Function, typename... Args>
    auto                LockRead(Function&& func, Args&&... args) noexcept -> decltype(func(std::forward<Args>(args)...));
    
    template <typename Function, typename... Args>
    auto                LockWrite(Function&& func, Args&&... args) noexcept -> decltype(func(std::forward<Args>(args)...));
    
private:
    
    std::shared_mutex   mMutex;
};

template <typename Function, typename... Args>
auto ReadWriteLock::LockRead(Function&& func, Args&&... args) noexcept -> decltype(func(std::forward<Args>(args)...))
{
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return func(std::forward<Args>(args)...);
}

template <typename Function, typename... Args>
auto ReadWriteLock::LockWrite(Function&& func, Args&&... args) noexcept -> decltype(func(std::forward<Args>(args)...))
{
    std::lock_guard<std::shared_mutex> lock(mMutex);
    return func(std::forward<Args>(args)...);
}

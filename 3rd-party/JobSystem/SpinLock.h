#pragma once

#include "boost/smart_ptr/detail/spinlock.hpp"

class SpinLock final
{
public:
    
    
    template <typename Function, typename... Args>
    auto                    Lock(Function&& func, Args&&... args) noexcept -> decltype(func(std::forward<Args>(args)...));
    
private:
    
    boost::detail::spinlock mMutex   {};
};

template <typename Function, typename... Args>
auto SpinLock::Lock(Function&& func, Args&&... args) noexcept -> decltype(func(std::forward<Args>(args)...))
{
    boost::detail::spinlock::scoped_lock lock(mMutex);
    return func(std::forward<Args>(args)...);
}

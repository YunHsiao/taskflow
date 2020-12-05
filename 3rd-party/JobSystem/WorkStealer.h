#pragma once

#include <memory>
#include <cstdint>
#include <atomic>

// 对于ForEach类的任务 可以将一个Job等分为互不依赖的N个Work
// 这里维护了每个Job要处理的Work的Start和End 并提供线程安全的读写
// 当一个Job处理完它自己的所有Work 不会立即结束 而是尝试为这次ForEach的其他Job去分担一部分Work
// 这个尝试去分担的过程 被称为工作窃取

struct alignas(64) WorkStealingRange final
{
    uint32_t                mWorkCountPerBatch;     // 4
    uint32_t                mJobCount;              // 8
    uint32_t                mWorkCount;             // 12
    uint32_t                mPhaseCount;            // 16
    std::atomic<uint32_t>*  mStartEndIndex;         // 24
    uint32_t*               mPhaseData;             // 32
    uint8_t                 mPadding[32];           // 为了让后面声明的变量不在同一条CacheLine 防止伪共享
};

class alignas(64) WorkStealer final
{
public:
    
    using Ptr               = std::shared_ptr<WorkStealer>;
    
                            WorkStealer() noexcept = default;
                            ~WorkStealer();
                            WorkStealer(WorkStealingRange const&) = delete;
                            WorkStealer(WorkStealingRange&&) = delete;
                            WorkStealer& operator=(WorkStealingRange const&) = delete;
                            WorkStealer& operator=(WorkStealingRange&&) = delete;

    uint32_t                Initial(uint32_t const workCount, uint32_t const batchSize) noexcept;
    bool                    TrySteal(uint16_t& start, uint16_t& end, uint32_t const jobIndex) noexcept;
    
private:
    
    WorkStealingRange       mRange          {};
    uint8_t*                mRangeBuffer    { nullptr };
};

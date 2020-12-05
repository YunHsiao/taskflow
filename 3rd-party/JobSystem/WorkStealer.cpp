#include "WorkStealer.h"
#include <algorithm>
#include "BatchAllocator.h"
#include "ThreadPool.h"

uint32_t constexpr kIntPerCacheLine = static_cast<uint32_t>(PLATFORM_CACHE_LINE_SIZE / sizeof(uint32_t));
uint32_t constexpr kBatchesPerPhase = 0xffff - 2;

struct WorkStealingAllocationData
{
    WorkStealingRange data;
};

void AllocateWorkStealingRange(WorkStealingAllocationData& outputRange,
                               BatchAllocator& allocator, uint32_t const iterationCount,
                               uint32_t batchSize, int32_t const forceWorkerThreadCount) noexcept
{
    batchSize = std::min(iterationCount, batchSize);
    batchSize = std::max(batchSize, 1U);

    uint32_t const numBatches                   = (iterationCount + batchSize - 1) / batchSize;
    uint32_t const batchSizeBasedThreadCount    = std::max(numBatches, 1U);
    uint32_t const workerThreadCount            = forceWorkerThreadCount != -1 ? forceWorkerThreadCount : std::min(batchSizeBasedThreadCount, ThreadPool::kMaxThreadCount + 1U);
    uint32_t const numPhases                    = std::max((numBatches + kBatchesPerPhase - 1) / kBatchesPerPhase, 1U);
    uint32_t const perThreadInts                = std::max(kIntPerCacheLine, numPhases);

    allocator.PadToCacheLine();
    allocator.Allocate(outputRange.data.mStartEndIndex, workerThreadCount * perThreadInts);
    allocator.Allocate(outputRange.data.mPhaseData, workerThreadCount * kIntPerCacheLine);

    outputRange.data.mWorkCountPerBatch         = batchSize;
    outputRange.data.mJobCount                  = workerThreadCount;
    outputRange.data.mWorkCount                 = iterationCount;
    outputRange.data.mPhaseCount                = numPhases;
}

void InitializeWorkStealingRange(WorkStealingRange& outputRange, WorkStealingAllocationData const& data) noexcept
{
    outputRange = data.data;

    uint32_t const perThreadInts    = std::max(kIntPerCacheLine, outputRange.mPhaseCount);
    uint32_t const numBatches       = (outputRange.mWorkCount + outputRange.mWorkCountPerBatch - 1) / outputRange.mWorkCountPerBatch;
    
    for (uint32_t phase = 0; phase < outputRange.mPhaseCount; ++phase)
    {
        uint32_t phaseBase = kBatchesPerPhase * phase;
        uint32_t phaseBatchCount = std::min(numBatches - phaseBase, kBatchesPerPhase);
        uint32_t countPerJob = phaseBatchCount / outputRange.mJobCount;
        
        for (uint32_t i = 0; i < outputRange.mJobCount; ++i)
        {
            outputRange.mStartEndIndex[i * perThreadInts + phase] = ((countPerJob * (i + 1)) << 16) | (countPerJob * i);
        }
        
        outputRange.mStartEndIndex[(outputRange.mJobCount - 1) * perThreadInts + phase] = (phaseBatchCount << 16) | ((outputRange.mJobCount - 1) * countPerJob);
    }
    
    for (uint32_t i = 0; i < outputRange.mJobCount; ++i)
    {
        outputRange.mPhaseData[kIntPerCacheLine * i] = 0;
    }
}

bool GetWorkStealingRange(uint32_t& beginIndex, uint32_t& endIndex, WorkStealingRange& range, uint32_t const jobIndex) noexcept
{
    uint32_t const perThreadInts    = std::max(kIntPerCacheLine, range.mPhaseCount);
    uint32_t const phase            = range.mPhaseData[jobIndex * kIntPerCacheLine];
    uint32_t const rangeStartEnd    = range.mStartEndIndex[jobIndex * perThreadInts + phase].fetch_add(1, std::memory_order_relaxed);
    uint32_t rangeEnd               = rangeStartEnd >> 16;
    uint32_t rangeStart             = rangeStartEnd & 0xffff;
    
    if (rangeStart >= rangeEnd)
    {
        for (uint32_t other = (jobIndex + 1) % range.mJobCount; other != jobIndex; other = (other + 1) % range.mJobCount)
        {
            uint32_t newStart           = 0;
            uint32_t newEnd             = 0;
            uint32_t otherRangeStartEnd = 0;
            uint32_t newOtherRange      = 0;
            
            do
            {
                otherRangeStartEnd = range.mStartEndIndex[other * perThreadInts + phase].load(std::memory_order_relaxed);
                uint32_t const otherRangeEnd = otherRangeStartEnd >> 16;
                uint32_t const otherRangeStart = otherRangeStartEnd & 0xffff;
                
                if (otherRangeStart >= otherRangeEnd)
                {
                    newStart    = 0;
                    newEnd      = 0;
                    break;
                }
                
                uint32_t const leaveSize    = (otherRangeEnd - otherRangeStart) >> 1;
                newStart                    = otherRangeStart + leaveSize;
                newEnd                      = otherRangeEnd;
                newOtherRange               = (newStart << 16) | otherRangeStart;
            }
            while (! range.mStartEndIndex[other * perThreadInts + phase].compare_exchange_weak(otherRangeStartEnd, newOtherRange, std::memory_order_relaxed, std::memory_order_relaxed));
            
            if (newEnd)
            {
                rangeStart  = newStart;
                rangeEnd    = newEnd;
                ++newStart;
                range.mStartEndIndex[jobIndex * perThreadInts + phase].store((newEnd << 16) | newStart, std::memory_order_relaxed);
                break;
            }
        }
        
        if (rangeStart >= rangeEnd)
        {
            if (phase + 1 < range.mPhaseCount)
            {
                ++range.mPhaseData[jobIndex * kIntPerCacheLine];
                return GetWorkStealingRange(beginIndex, endIndex, range, jobIndex);
            }
            
            beginIndex  = 0;
            endIndex    = 0;
            return false;
        }
    }
    
    beginIndex  = (rangeStart + kBatchesPerPhase * phase) * range.mWorkCountPerBatch;
    endIndex    = std::min(beginIndex + range.mWorkCountPerBatch, range.mWorkCount);
    return true;
}

WorkStealer::~WorkStealer()
{
    BatchAllocator::DeallocateRoot(mRangeBuffer);
}

uint32_t WorkStealer::Initial(uint32_t const workCount, uint32_t const batchSize) noexcept
{
    BatchAllocator allocator;
    WorkStealingAllocationData rangeAllocation;

    uint32_t* root = nullptr;
    allocator.AllocateRoot(root);

    AllocateWorkStealingRange(rangeAllocation, allocator, workCount, batchSize, -1);
    allocator.Commit();
    mRangeBuffer = reinterpret_cast<uint8_t*>(root);

    InitializeWorkStealingRange(mRange, rangeAllocation);
    return mRange.mJobCount;
}

bool WorkStealer::TrySteal(uint16_t& start, uint16_t& end, uint32_t const jobIndex) noexcept
{
    uint32_t x, y;
    bool const succeed = GetWorkStealingRange(x, y, mRange, jobIndex);
    assert(x < 0xffff && y < 0xffff);
    start = static_cast<uint16_t>(x);
    end = static_cast<uint16_t>(y);
    return succeed;
}

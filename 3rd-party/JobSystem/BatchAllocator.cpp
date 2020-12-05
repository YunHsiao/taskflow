#include "BatchAllocator.h"
#include <algorithm>

BatchAllocator::BatchAllocator()
{
    mBufferSize = 0;
    mAllocationCount = 0;
    mMaxAlignment = 4;
}

void BatchAllocator::AllocateInternal(void** dstPtr, int32_t rootIndex, size_t elementSize, size_t count, size_t alignment)
{
    assert(elementSize >= alignment || elementSize == 1);
    assert(elementSize % alignment == 0 || elementSize == 1);

//    AssertMsg(mAllocationCount < kMaxAllocationCount, "kMaxAllocationCount exceeded. This will crash. Please increase kMaxAllocationCount.");

    size_t size = elementSize * count;

    if (rootIndex != -1)
    {
        #if ENABLE_ASSERTIONS
        assert(reinterpret_cast<size_t>(dstPtr) < mAllocations[rootIndex].mOffset + mAllocations[rootIndex].mSize);
        #endif
        assert(reinterpret_cast<size_t>(dstPtr) >= mAllocations[rootIndex].mOffset);
    }
    else
    {
        assert(reinterpret_cast<size_t>(dstPtr) >= mBufferSize);
    }

    mAllocations[mAllocationCount].mDstPtr = dstPtr;
    mAllocations[mAllocationCount].mRootIndex = rootIndex;
    mAllocations[mAllocationCount].mSrcSize = 0;
    #if ENABLE_ASSERTIONS
    mAllocations[mAllocationCount].mSize = size;
    #endif

    mBufferSize = Align(mBufferSize, alignment);
    mAllocations[mAllocationCount].mOffset = mBufferSize;
    mBufferSize += size;

    mMaxAlignment = MaxAlignment(mMaxAlignment, alignment);

    mAllocationCount++;
}

void BatchAllocator::PadToCacheLine()
{
    mBufferSize = Align(mBufferSize, PLATFORM_CACHE_LINE_SIZE);
    mMaxAlignment = MaxAlignment(mMaxAlignment, PLATFORM_CACHE_LINE_SIZE);
}

void BatchAllocator::ReallocateInternal(void** dstPtr, size_t elementSize, size_t newCount, size_t oldCount, size_t alignment)
{
    AllocateInternal(dstPtr, -1, elementSize, newCount, alignment);
    mAllocations[mAllocationCount - 1].mSrcSize = std::min(oldCount, newCount) * elementSize;
}

void BatchAllocator::Commit()
{
    uint8_t* buffer = reinterpret_cast<uint8_t*>(AlignedAllocate(mBufferSize, mMaxAlignment));

    for (size_t i = 0; i != mAllocationCount; i++)
    {
        void* ptr = buffer + mAllocations[i].mOffset;
        size_t dstOffset = 0;
        int32_t rootIndex = mAllocations[i].mRootIndex;
        if (rootIndex != -1)
            dstOffset = (size_t)buffer + mAllocations[rootIndex].mOffset;

        uint8_t* dstPtr = reinterpret_cast<uint8_t*>(mAllocations[i].mDstPtr) + dstOffset;
        if (mAllocations[i].mSrcSize != 0)
            memcpy(ptr, *(reinterpret_cast<void**>(dstPtr)), mAllocations[i].mSrcSize);

        *(reinterpret_cast<void**>(dstPtr)) = ptr;
    }
}

//@TODO: Test for AllocateField with 0 size, allocating no memory.

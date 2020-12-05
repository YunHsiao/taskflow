#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstddef>
#include <cassert>
#include "Memory.h"

#define ENABLE_ASSERTIONS 1

class BatchAllocator
{
public:
    BatchAllocator();

    void Commit();

    // Allocates the root object
    template<class T>
    void AllocateRoot(T*& dstPtr, size_t count = 1, size_t alignment = 0);

    // Allocates a field (the ptr passed into this function must be relative to the root)
    template<class T>
    void AllocateField(T*& dstPtr, size_t count, size_t alignment = 0);
    void AllocateField(void*& dstPtr, size_t count, size_t alignment, size_t sizeOf);

    // Allocates an o
    template<class T>
    void Allocate(T*& dstPtr, size_t count, size_t alignment = 0);

    // Allocates a new array and memcppies min(srcCount, newCount) elements,
    // then assigns the new ptr location to passed adress.
    template<class T>
    void Reallocate(T*& dstPtr, size_t newCount, size_t srcCount, size_t alignment = 0);

    // Aligns the next allocation to the platform's cache line size.
    // Use to avoid false sharing between jobs.
    void PadToCacheLine();

    static void DeallocateRoot(void* dstPtr) { free(dstPtr); }

private:

    void AllocateInternal(void** dstPtr, int32_t rootIndex, size_t elementSize, size_t count, size_t alignment);
    void ReallocateInternal(void** dstPtr, size_t elementSize, size_t newCount, size_t oldCount, size_t alignment);

    struct Allocation
    {
        void**  mDstPtr;

        int32_t mRootIndex;
        size_t  mOffset;
        size_t  mSrcSize;
        #if ENABLE_ASSERTIONS
        size_t  mSize;
        #endif
    };

    enum { kMaxAllocationCount = 64 };

    size_t          mBufferSize;
    size_t          mAllocationCount;
    size_t          mMaxAlignment;
    Allocation      mAllocations[kMaxAllocationCount];
};

template<class T>
void BatchAllocator::AllocateRoot(T*& dstPtr, size_t count, size_t alignment)
{
    // NOTE: For now we assume that first allocation is root allocation
    // If we have use cases for multiple root allocations, then we need to genaralize this code a bit more...
    typedef typename std::remove_const<T>::type MutableT;
    MutableT *&dstPtrMutable = const_cast<MutableT *&>(dstPtr);
    assert((void*)&dstPtr == (void*)&dstPtrMutable);

    assert(mAllocationCount == 0);
    alignment = (alignment != 0) ? alignment : alignof(MutableT);

    AllocateInternal(reinterpret_cast<void**>(&dstPtrMutable), -1, sizeof(MutableT), count, alignment);

    dstPtrMutable = NULL;
}

template<class T>
void BatchAllocator::Allocate(T*& dstPtr, size_t count, size_t alignment)
{
    // NOTE: For now we assume that first allocation is root allocation
    // If we have use cases for multiple root allocations, then we need to genaralize this code a bit more...
    typedef typename std::remove_const<T>::type MutableT;
    MutableT *&dstPtrMutable = const_cast<MutableT *&>(dstPtr);
    assert((void*)&dstPtr == (void*)&dstPtrMutable);

    assert(mAllocationCount >= 1);
    alignment = (alignment != 0) ? alignment : alignof(MutableT);

    AllocateInternal(reinterpret_cast<void**>(&dstPtrMutable), -1, sizeof(MutableT), count, alignment);
}

template<class T>
void BatchAllocator::Reallocate(T*& dstPtr, size_t newCount, size_t srcCount, size_t alignment)
{
    // NOTE: For now we assume that first allocation is root allocation
    // If we have use cases for multiple root allocations, then we need to genaralize this code a bit more...
    typedef typename std::remove_const<T>::type MutableT;
    MutableT *&dstPtrMutable = const_cast<MutableT *&>(dstPtr);
    assert((void*)&dstPtr == (void*)&dstPtrMutable);

    alignment = (alignment != 0) ? alignment : alignof(MutableT);

    ReallocateInternal(reinterpret_cast<void**>(&dstPtrMutable), sizeof(MutableT), newCount, srcCount, alignment);
}

template<class T>
void BatchAllocator::AllocateField(T*& dstPtr, size_t count, size_t alignment)
{
    // NOTE: For now we assume that first allocation is root allocation
    // If we have use cases for multiple root allocations, then we need to genaralize this code a bit more...
    assert(mAllocationCount >= 1);

    typedef typename std::remove_const<T>::type MutableT;
    MutableT *&dstPtrMutable = const_cast<MutableT *&>(dstPtr);
    assert((void*)&dstPtr == (void*)&dstPtrMutable);

    alignment = (alignment != 0) ? alignment : alignof(MutableT);

    AllocateInternal(reinterpret_cast<void**>(&dstPtrMutable), 0, sizeof(MutableT), count, alignment);
}

inline void BatchAllocator::AllocateField(void*& dstPtr, size_t count, size_t alignment, size_t sizeOf)
{
    // NOTE: For now we assume that first allocation is root allocation
    // If we have use cases for multiple root allocations, then we need to genaralize this code a bit more...
    assert(mAllocationCount >= 1);

    alignment = (alignment != 0) ? alignment : sizeOf;

    AllocateInternal(&dstPtr, 0, sizeOf, count, alignment);
}

#include "Memory.h"
#include <algorithm>
#include <cstdint>

#define Assert assert
#define AssertMsg(test, msg, ...)

bool constexpr is_power_of_two(size_t input)
{
    return input != 0 && (input & (input - 1)) == 0;
}

template<typename PtrType>
inline bool is_aligned_to(PtrType* value, size_t alignment)
{
    AssertMsg(is_power_of_two(alignment), "Alignment value must be a power of two");
    return (reinterpret_cast<intptr_t>(value) & (alignment - 1)) == 0;
}

template<typename IntegralType>
inline bool is_aligned_to(IntegralType value, size_t alignment)
{
    AssertMsg(is_power_of_two(alignment), "Alignment value must be a power of two");
    return (static_cast<size_t>(value) & (alignment - 1)) == 0;
}

template<typename PtrType>
bool constexpr is_aligned(PtrType* value)
{
    return is_aligned_to(value, alignof(PtrType));
}

namespace memory_impl
{
    template<typename DestPtrType, typename SrcType>
    struct safe_ptr_to_ptr_cast_impl
    {
        inline static DestPtrType* cast(SrcType* input)
        {
            AssertMsg(is_aligned_to(input, alignof(DestPtrType)), "reinterpret_cast would result in an unaligned pointer");
            return reinterpret_cast<DestPtrType*>(input);
        }
    };

    template<typename SrcType>
    struct safe_ptr_to_ptr_cast_impl<void, SrcType>
    {
        static void constexpr* cast(SrcType* input) { return input; }
    };

    template<typename DestPtrType, typename SrcType>
    struct safe_int_to_ptr_cast_impl
    {
        inline static DestPtrType* cast(SrcType input)
        {
            AssertMsg(is_aligned_to(input, alignof(DestPtrType)), "reinterpret_cast would result in an unaligned pointer");
            return reinterpret_cast<DestPtrType*>(input);
        }
    };

    template<typename SrcType>
    struct safe_int_to_ptr_cast_impl<void, SrcType>
    {
        static void constexpr* cast(SrcType input) { return reinterpret_cast<void*>(input); }
    };
}

template<typename DestPtrType, typename SrcType>
inline DestPtrType* safe_ptr_cast(SrcType* input)
{
    return memory_impl::safe_ptr_to_ptr_cast_impl<DestPtrType, SrcType>::cast(input);
}

template<typename DestPtrType, typename SrcType>
inline DestPtrType* safe_ptr_cast(SrcType input)
{
    return memory_impl::safe_int_to_ptr_cast_impl<DestPtrType, SrcType>::cast(input);
}

template<typename OutputPtrType, typename InputPtrType, typename OffsetType>
inline OutputPtrType* add_offset_to_ptr(InputPtrType* ptr, OffsetType offset)
{
    return safe_ptr_cast<OutputPtrType>(reinterpret_cast<uintptr_t>(ptr) + offset);
}

template<typename PtrType>
inline PtrType* align_to(PtrType* value, size_t alignment)
{
    AssertMsg(is_poweradd_offset_to_ptr_of_two(alignment), "Alignment value must be a power of two");
    return reinterpret_cast<PtrType*>((reinterpret_cast<intptr_t>(value) + (alignment - 1)) & ~(alignment - 1));
}

template<typename IntegralType>
inline IntegralType align_to(IntegralType value, size_t alignment)
{
    AssertMsg(is_power_of_two(alignment), "Alignment value must be a power of two");
    return static_cast<IntegralType>((static_cast<size_t>(value) + (alignment - 1)) & ~(alignment - 1));
}


void* AlignedAllocate(size_t size, size_t alignment)
{
    size_t const aligned_size = Align(size, alignment);
    void* ptr = nullptr;

#if defined(_WINDOWS)
    ptr = _aligned_malloc(aligned_size, alignment);
#elif defined(__APPLE__)
    ptr = nullptr;
    posix_memalign(&ptr, std::max<size_t>(alignment, sizeof(void*)), aligned_size);
#elif defined(__ANDROID__)
    // Don't bother using aligned_size here, as we're doing custom alignment, just mark the var as unused.
    (void)aligned_size;
    alignment = std::max<size_t>(std::max<size_t>(alignment, sizeof(void*)), sizeof(size_t));
    size_t const padded_size = size + alignment + sizeof(size_t);
    ptr = malloc(padded_size);
    if (ptr != nullptr)
    {
        void const* allocated_ptr = ptr;
        ptr = align_to(add_offset_to_ptr<void>(ptr, sizeof(size_t)), alignment);

        size_t const padding_size = static_cast<size_t>(reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(allocated_ptr));
        size_t* padding_size_ptr = add_offset_to_ptr<size_t>(ptr, -sizeof(size_t));
        *padding_size_ptr = padding_size;
    }
#else
    ptr = aligned_alloc(alignment, aligned_size);
#endif

    return ptr;
}
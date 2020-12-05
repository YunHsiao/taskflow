#include "Memory.h"
#include <algorithm>

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
    const size_t padded_size = size + alignment + sizeof(size_t);
    ptr = malloc(padded_size);
    if (ptr != nullptr)
    {
        const void* allocated_ptr = ptr;
        ptr = align_to(add_offset_to_ptr<void>(ptr, sizeof(size_t)), alignment);

        const size_t padding_size = safe_static_cast<size_t>(reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(allocated_ptr));
        size_t* padding_size_ptr = add_offset_to_ptr<size_t>(ptr, -sizeof(size_t));
        *padding_size_ptr = padding_size;
    }
#else
    ptr = aligned_alloc(alignment, aligned_size);
#endif
    
    return ptr;
}

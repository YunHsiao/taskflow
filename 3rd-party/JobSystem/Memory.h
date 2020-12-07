#pragma once

#include <cstddef>
#include <cstdint>

#ifndef PLATFORM_CACHE_LINE_SIZE
    #define PLATFORM_CACHE_LINE_SIZE 64
#endif

template<typename T>
inline T constexpr Align(T size, size_t alignment) noexcept
{
    return ((size + (alignment - 1)) & ~(alignment - 1));
}

/// Return maximum alignment. Both alignments must be 2^n.
inline size_t MaxAlignment(size_t alignment1, size_t alignment2) noexcept
{
    return ((alignment1 - 1) | (alignment2 - 1)) + 1;
}

void* AlignedAllocate(size_t size, size_t alignment = 16);

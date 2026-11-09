// Build-time configuration.
//
// The cache line size is a property of the target CPU, not of this library,
// so it is not hard-coded here. It is supplied by the build system:
//
//   cmake -DMPMC_RING_CACHE_LINE_SIZE=128 ...
//
// and reaches every consumer through the INTERFACE compile definitions of the
// mpmc_ring::mpmc_ring target. All translation units of one program must see
// the same value, otherwise class layouts differ between them (an ODR
// violation); propagating it through the target is what guarantees that.
#pragma once

#include <bit>
#include <cstddef>

#ifndef MPMC_RING_CACHE_LINE_SIZE
#  error "MPMC_RING_CACHE_LINE_SIZE is not defined. Link against the CMake target mpmc_ring::mpmc_ring or pass -DMPMC_RING_CACHE_LINE_SIZE=<bytes>."
#endif

namespace mpmc_ring {

inline constexpr std::size_t kCacheLine = MPMC_RING_CACHE_LINE_SIZE;

static_assert(std::has_single_bit(kCacheLine),
              "MPMC_RING_CACHE_LINE_SIZE must be a power of two");
static_assert(kCacheLine >= 16, "MPMC_RING_CACHE_LINE_SIZE is implausibly small");

}  // namespace mpmc_ring

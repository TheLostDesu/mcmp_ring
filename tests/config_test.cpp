// Compile-time check that the cache line size reaches the code from CMake.
#include <mpmc_ring/config.hpp>

static_assert(mpmc_ring::kCacheLine == MPMC_RING_CACHE_LINE_SIZE);

int main() { return 0; }

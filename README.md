# mpmc_ring

A header-only C++20 multi-producer / multi-consumer **overwriting** ring buffer:
`enqueue` never fails and never blocks. When the ring is full, the oldest
element is dropped.

> **Status: work in progress.** Only the build scaffolding is in place.

Based on Lee T.H.'s article [Put a ring on it](https://h4x0r.org/ring/) and
the accompanying C implementation,
[h4x0r/mpmc_lf_ring](https://codeberg.org/h4x0r/mpmc_lf_ring). This is an
independent C++20 implementation, not a fork.

## Cache line size

The cache line size is a property of the target CPU (64 bytes on x86-64, 128
on Apple M-series), so it is a build parameter rather than a constant in the
code:

```sh
cmake -B build -DMPMC_RING_CACHE_LINE_SIZE=128
```

The default is 64. The value reaches every consumer through the `INTERFACE`
compile definitions of the `mpmc_ring::mpmc_ring` target, so all translation
units agree on the layout. Including the headers without it is a compile
error.

## Build

```sh
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Requires a C++20 compiler (tested with GCC 13 and Clang 18) and CMake 3.20+.

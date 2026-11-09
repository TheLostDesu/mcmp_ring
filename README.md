# mpmc_ring

A header-only C++20 multi-producer / multi-consumer **overwriting** ring buffer:
`enqueue` never fails and never blocks. When the ring is full, the oldest
element is dropped.

> **Status: work in progress.** The build scaffolding is in place; the ring
> itself is being written. Nothing here is benchmarked yet, so there are no
> performance claims in this README.

## Why

Most bounded lock-free queues (Vyukov's MPMC queue, DPDK's `rte_ring`,
`boost::lockfree::queue`) share one contract: when the queue is full, the
producer is told "no" and has to decide what to do — spin, drop the new item,
or back-pressure whatever is upstream.

For a class of workloads that is the wrong default. Telemetry, tracing, debug
logging out of a packet-processing thread, conflated market data: in all of
these the *newest* data is the valuable part, and the producer is a hot path
that must not wait for a slow consumer. What you want is the opposite contract:
the producer always succeeds, and a consumer that falls behind loses the oldest
entries.

I work on a DPDK data plane, where `rte_ring` is the tool you reach for first,
and wanted to understand how the drop-oldest contract can be implemented
without locks for many producers and many consumers at once — which turns out
to be noticeably harder than the reject-when-full case.

## Background and credit

The starting point is Lee T.H.'s article
[Put a ring on it](https://h4x0r.org/ring/) and the accompanying C
implementation, [h4x0r/mpmc_lf_ring](https://codeberg.org/h4x0r/mpmc_lf_ring).
The core idea comes from there:

* Enqueue and dequeue positions ("epochs") live in a single 128-bit atomic.
  A producer takes a ticket with a fetch-add and CASes its value into the cell
  for that epoch; a consumer does the same on its side.
* Each cell carries the epoch it was written in, so a thread that was
  descheduled and got lapped can tell, and retries with a fresh ticket.
* When producers outrun consumers, producers move the dequeue epoch forward.
  That is what "dropping the oldest" means.

This repository is **not a fork**. It is an independent C++20 implementation
written after studying that code, with different trade-offs (below). The
original is the reference to benchmark against.

## Design goals

Where this implementation intends to differ from the reference:

| | Reference (C) | This library |
|---|---|---|
| Element type | `void *` or `memcpy` of a runtime size | `template <T>`, constrained by a concept |
| Entry layout | header + runtime-sized payload | header + `T` fit in exactly one cache line, checked at compile time |
| Cache line size | n/a | build parameter (`MPMC_RING_CACHE_LINE_SIZE`), not a constant in the code |
| Memory ordering | to be compared | explicit per operation, each one justified in a comment |
| Full ring | producers back off by sleeping | no syscalls on the enqueue path |
| Verification | not reviewed yet | stress tests under ThreadSanitizer, in CI |

### Why the element type is constrained

An overwriting ring drops values. There is no well-defined point — or thread —
at which a destructor for a dropped value could run, and values are moved in
and out with byte copies. So elements must be trivially copyable and trivially
destructible; `std::string` or `std::unique_ptr` are rejected at compile time
rather than leaking at run time.

The size limit follows from the layout: one entry is one cache line, so an
operation touches a single line of payload and neighbouring entries cannot
false-share. The usable payload is therefore the line size *minus the entry
header*, not the full line.

### Why the cache line size is a build parameter

It is a property of the target CPU (64 bytes on x86-64, 128 on Apple
M-series), not of the library. `std::hardware_destructive_interference_size`
exists, but GCC itself warns that its value can change with compiler flags,
which makes it unsafe to bake into a class layout. Instead the value is set in
CMake and propagated to every consumer through the target's `INTERFACE`
compile definitions, so all translation units agree on the layout. Including
the headers without it is a hard `#error`, not a silent default.

## Open questions

Things not settled yet, tracked here so the reasoning is visible:

* **128-bit fetch-add.** x86-64 has no `lock xadd` for 16 bytes; the
  compiler emits a `cmpxchg16b` loop (or calls libatomic). Producers and
  consumers also contend on that one word. Is one shared 128-bit word better
  or worse than two 64-bit counters on separate lines, and what does
  correctness need?
* **Behaviour near full.** With the sleep removed, what bounds the retries of
  a producer that keeps getting lapped?
* **Progress guarantee.** Which operations are lock-free, and is anything
  wait-free? To be stated precisely once the algorithm is final.

## Roadmap

- [x] Build scaffolding, cache line size as a build parameter, CI
- [ ] Element-type concept (`Cacheable`)
- [ ] Word ring: values that fit in 8 bytes, stored inside the cell
- [ ] `OverwriteRing<T>`: one cache line per entry
- [ ] Stress tests under TSan / ASan / UBSan
- [ ] Latency benchmarks (p50 / p99 / p99.9) against the C reference and a
      reject-when-full Vyukov queue

## Build

```sh
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo   # add -DMPMC_RING_CACHE_LINE_SIZE=128 for Apple silicon
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Requires a C++20 compiler (tested with GCC 13 and Clang 18) and CMake 3.20+.

## References

* Lee T.H., [Put a ring on it](https://h4x0r.org/ring/)
* Dmitry Vyukov, [Bounded MPMC queue](https://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue)
* DPDK, [Ring library](https://doc.dpdk.org/guides/prog_guide/ring_lib.html)

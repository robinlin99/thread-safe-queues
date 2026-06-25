# Queue Implementations

Three queue implementations in C++17, ranging from a simple single-threaded growable queue to thread-safe bounded queues with lock-based and lock-free synchronization.

## Implementations

| File | Class | Thread-safe | Bounded | Description |
|------|-------|-------------|---------|-------------|
| `single_threaded_queue.hpp` | `VectorQueue<T>` | No | No | Circular buffer backed by a dynamically-growing `std::vector` |
| `queue_lock.hpp` | `LockQueue<T, Capacity>` | Yes (MPMC) | Yes | Fixed-capacity ring buffer protected by a mutex + condition variables |
| `queue_lock_free.hpp` | `SPSCQueueLockFree<T, Capacity>` | Yes (SPSC only) | Yes | Fixed-capacity ring buffer using atomic acquire/release — no locks |

### VectorQueue

Single-threaded use only. Starts at a caller-supplied initial capacity and doubles automatically when full. Provides `front()`, `back()`, `push()`, `pop()`, and `clear()`.

```cpp
VectorQueue<int> q(4);   // initial capacity 4
q.push(1);
q.push(2);
int v;
q.pop(v);   // v == 1
```

### LockQueue

Thread-safe for any number of producers and consumers. Fixed capacity set at compile time. Offers both blocking (`push`/`pop`) and non-blocking (`try_push`/`try_pop`) variants.

```cpp
LockQueue<int, 64> q;
q.push(42);        // blocks if full
q.try_push(42);    // returns false if full
```

### SPSCQueueLockFree

Lock-free, but strictly single-producer / single-consumer. Uses cache-line-aligned atomics with acquire/release ordering to eliminate false sharing. Non-blocking only.

```cpp
SPSCQueueLockFree<int, 64> q;
q.try_push(42);    // returns false if full
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run Benchmarks

```bash
./build/bench_lock
./build/bench_lock_free
```

## Run Sample Application

Demonstrates `VectorQueue` growing from an initial capacity of 4 up to 64 elements through repeated buffer doublings:

```bash
./build/sample_single_threaded_queue
```

## Unit Tests

Tests live in `tests/` and use the single-header [doctest](https://github.com/doctest/doctest) framework.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

## Benchmark Details

### bench_lock

Uses blocking `push`/`pop` with mutex + condition variables.

| Scenario | Description |
|----------|-------------|
| SPSC | 1 producer, 1 consumer |
| MPSC (2) | 2 producers, 1 consumer |
| MPSC (4) | 4 producers, 1 consumer |

### bench_lock_free

Uses spin-retry `try_push`/`try_pop` (single producer, single consumer only).

| Scenario | Description |
|----------|-------------|
| SPSC (tight spin) | No yielding, maximum throughput |
| SPSC (yield every N) | Periodic `yield()` to simulate batched workloads |

## Configuration

Edit the constants at the top of each benchmark file:

- `QUEUE_CAPACITY` — ring buffer size (default: 1024)
- `NUM_ITEMS` — total items pushed/popped per run (default: 1,000,000)

## Requirements

- C++17 compiler
- CMake 3.16+
- pthreads

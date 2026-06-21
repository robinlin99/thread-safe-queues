# Queue Benchmark

Performance benchmarks comparing a mutex/condition-variable queue (`LockQueue`) against a lock-free SPSC queue (`SPSCQueueLockFree`).

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run

```bash
./build/bench_lock
./build/bench_lock_free
```
## Unit Tests

Tests live in `tests/` and use the single-header doctest (`tests/doctest/doctest.h`).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

## Benchmarks

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

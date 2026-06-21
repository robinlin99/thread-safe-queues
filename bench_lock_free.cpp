/**
 * @file bench_lock_free.cpp
 * @brief Benchmark for the lock-free SPSC (single-producer / single-consumer) queue.
 *
 * Measures throughput (ops/sec) under a tight spin-retry loop and under
 * batched variants that periodically yield the CPU. Because the queue is
 * strictly SPSC, only one producer and one consumer thread are used.
 */

#include "queue_lock_free.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

/// Ring buffer capacity (max elements - 1 usable due to sentinel slot).
static constexpr std::size_t QUEUE_CAPACITY = 1024;

/// Total number of items pushed and popped per benchmark run.
static constexpr std::size_t NUM_ITEMS = 1'000'000;

using Queue = SPSCQueueLockFree<std::uint64_t, QUEUE_CAPACITY>;

/**
 * @brief Pushes sequential integers into the queue via spin-retry.
 * @param q     Reference to the shared queue.
 * @param count Number of items to produce.
 *
 * Calls try_push() in a tight loop until it succeeds. No OS-level blocking
 * occurs — the thread busy-waits when the queue is full.
 */
void producer(Queue& q, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        while (!q.try_push(static_cast<std::uint64_t>(i))) {
            // spin until slot available
        }
    }
}

/**
 * @brief Pops items from the queue via spin-retry.
 * @param q     Reference to the shared queue.
 * @param count Number of items to consume.
 *
 * Calls try_pop() in a tight loop until it succeeds. No OS-level blocking
 * occurs — the thread busy-waits when the queue is empty.
 */
void consumer(Queue& q, std::size_t count) {
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < count; ++i) {
        while (!q.try_pop(value)) {
            // spin until data available
        }
    }
}

/**
 * @brief Pure spin SPSC benchmark — maximum achievable throughput.
 *
 * Both producer and consumer spin without yielding, giving an upper bound
 * on throughput for this queue under ideal conditions (dedicated cores).
 */
void benchmark_spsc() {
    Queue q;

    auto start = std::chrono::high_resolution_clock::now();

    std::thread prod(producer, std::ref(q), NUM_ITEMS);
    std::thread cons(consumer, std::ref(q), NUM_ITEMS);

    prod.join();
    cons.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double throughput = static_cast<double>(NUM_ITEMS) / (elapsed.count() / 1e6);

    std::cout << "=== Lock-Free Queue Benchmark (SPSC) ===\n";
    std::cout << "  Items:      " << NUM_ITEMS << "\n";
    std::cout << "  Capacity:   " << QUEUE_CAPACITY << "\n";
    std::cout << "  Time:       " << elapsed.count() << " us\n";
    std::cout << "  Throughput: " << static_cast<std::uint64_t>(throughput) << " ops/sec\n";
    std::cout << "\n";
}

/**
 * @brief SPSC benchmark with periodic yield to simulate batched workloads.
 * @param batch_size Number of items between each yield() call.
 *
 * Inserting yield() calls approximates a real workload where the CPU does
 * other work between queue operations. Lower batch_size = more contention
 * pauses; higher batch_size = closer to the tight-spin result.
 */
void benchmark_spsc_batch(std::size_t batch_size) {
    Queue q;

    auto start = std::chrono::high_resolution_clock::now();

    std::thread prod([&q, batch_size]() {
        for (std::size_t i = 0; i < NUM_ITEMS; ++i) {
            while (!q.try_push(static_cast<std::uint64_t>(i))) {
                // spin
            }
            // Simulate batched work pattern: yield periodically
            if (i % batch_size == 0) {
                std::this_thread::yield();
            }
        }
    });

    std::thread cons([&q, batch_size]() {
        std::uint64_t value = 0;
        for (std::size_t i = 0; i < NUM_ITEMS; ++i) {
            while (!q.try_pop(value)) {
                // spin
            }
            if (i % batch_size == 0) {
                std::this_thread::yield();
            }
        }
    });

    prod.join();
    cons.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double throughput = static_cast<double>(NUM_ITEMS) / (elapsed.count() / 1e6);

    std::cout << "SPSC with yield every " << batch_size << " items\n";
    std::cout << "  Items:      " << NUM_ITEMS << "\n";
    std::cout << "  Capacity:   " << QUEUE_CAPACITY << "\n";
    std::cout << "  Time:       " << elapsed.count() << " us\n";
    std::cout << "  Throughput: " << static_cast<std::uint64_t>(throughput) << " ops/sec\n";
    std::cout << "\n";
}

int main() {
    benchmark_spsc();
    benchmark_spsc_batch(64);
    benchmark_spsc_batch(256);
    benchmark_spsc_batch(1024);

    return 0;
}

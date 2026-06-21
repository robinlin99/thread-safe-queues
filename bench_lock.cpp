/**
 * @file bench_lock.cpp
 * @brief Benchmark for the mutex/condition-variable based LockQueue.
 *
 * Measures throughput (ops/sec) under single-producer/single-consumer (SPSC)
 * and multi-producer/single-consumer (MPSC) scenarios. The lock-based queue
 * supports multiple concurrent writers safely, so we exercise that here.
 */

#include "queue_lock.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

/// Ring buffer capacity (max elements - 1 usable due to sentinel slot).
static constexpr std::size_t QUEUE_CAPACITY = 1024;

/// Total number of items pushed and popped per benchmark run.
static constexpr std::size_t NUM_ITEMS = 1'000'000;

using Queue = LockQueue<std::uint64_t, QUEUE_CAPACITY>;

/**
 * @brief Pushes sequential integers into the queue.
 * @param q     Reference to the shared queue.
 * @param count Number of items to produce.
 *
 * Uses blocking push() — the thread will wait on the condition variable
 * if the queue is full.
 */
void producer(Queue& q, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        q.push(static_cast<std::uint64_t>(i));
    }
}

/**
 * @brief Pops items from the queue until count items have been consumed.
 * @param q     Reference to the shared queue.
 * @param count Number of items to consume.
 *
 * Uses blocking pop() — the thread will wait on the condition variable
 * if the queue is empty.
 */
void consumer(Queue& q, std::size_t count) {
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < count; ++i) {
        q.pop(value);
    }
}

/**
 * @brief Benchmarks single-producer / single-consumer throughput.
 *
 * Spawns one producer and one consumer thread, each processing NUM_ITEMS.
 * Reports wall-clock time and derived throughput.
 */
void benchmark_single_producer_single_consumer() {
    Queue q;

    auto start = std::chrono::high_resolution_clock::now();

    std::thread prod(producer, std::ref(q), NUM_ITEMS);
    std::thread cons(consumer, std::ref(q), NUM_ITEMS);

    prod.join();
    cons.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double throughput = static_cast<double>(NUM_ITEMS) / (elapsed.count() / 1e6);

    std::cout << "=== Lock-Based Queue Benchmark ===\n";
    std::cout << "SPSC (single producer / single consumer)\n";
    std::cout << "  Items:      " << NUM_ITEMS << "\n";
    std::cout << "  Capacity:   " << QUEUE_CAPACITY << "\n";
    std::cout << "  Time:       " << elapsed.count() << " us\n";
    std::cout << "  Throughput: " << static_cast<std::uint64_t>(throughput) << " ops/sec\n";
    std::cout << "\n";
}

/**
 * @brief Benchmarks multi-producer / single-consumer throughput.
 * @param num_producers Number of producer threads to spawn.
 *
 * Divides NUM_ITEMS evenly across producers. A single consumer drains
 * the full total. Demonstrates contention cost on the mutex as producer
 * count increases.
 */
void benchmark_multi_producer_single_consumer(std::size_t num_producers) {
    Queue q;
    std::size_t items_per_producer = NUM_ITEMS / num_producers;
    std::size_t total_items = items_per_producer * num_producers;

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> producers;
    producers.reserve(num_producers);
    for (std::size_t i = 0; i < num_producers; ++i) {
        producers.emplace_back(producer, std::ref(q), items_per_producer);
    }

    std::thread cons(consumer, std::ref(q), total_items);

    for (auto& t : producers) {
        t.join();
    }
    cons.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double throughput = static_cast<double>(total_items) / (elapsed.count() / 1e6);

    std::cout << "MPSC (" << num_producers << " producers / 1 consumer)\n";
    std::cout << "  Items:      " << total_items << "\n";
    std::cout << "  Capacity:   " << QUEUE_CAPACITY << "\n";
    std::cout << "  Time:       " << elapsed.count() << " us\n";
    std::cout << "  Throughput: " << static_cast<std::uint64_t>(throughput) << " ops/sec\n";
    std::cout << "\n";
}

int main() {
    benchmark_single_producer_single_consumer();
    benchmark_multi_producer_single_consumer(2);
    benchmark_multi_producer_single_consumer(4);

    return 0;
}

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "tests/doctest/doctest.h"

#include "queue_lock.hpp"
#include <atomic>
#include <string>
#include <thread>
#include <vector>

// Capacity=5 → 4 usable slots (one sentinel slot reserved).
static constexpr std::size_t CAP = 5;

// ---------------------------------------------------------------------------
// Single-threaded: state queries
// ---------------------------------------------------------------------------

TEST_CASE("isEmpty and isFull on a new queue") {
    LockQueue<int, CAP> q;
    CHECK(q.isEmpty());
    CHECK_FALSE(q.isFull());
}

// ---------------------------------------------------------------------------
// Single-threaded: try_push / try_pop
// ---------------------------------------------------------------------------

TEST_CASE("try_push (copy) and try_pop - basic round-trip") {
    LockQueue<int, CAP> q;
    int v = 0;
    CHECK(q.try_push(1));
    CHECK_FALSE(q.isEmpty());
    CHECK(q.try_pop(v));
    CHECK(v == 1);
    CHECK(q.isEmpty());
}

TEST_CASE("try_push (move) and try_pop - basic round-trip") {
    LockQueue<std::string, CAP> q;
    std::string s = "hello";
    CHECK(q.try_push(std::move(s)));
    std::string out;
    CHECK(q.try_pop(out));
    CHECK(out == "hello");
}

TEST_CASE("copy semantics - original value unchanged after try_push") {
    LockQueue<std::string, CAP> q;
    std::string s = "original";
    q.try_push(s);
    CHECK(s == "original");
    std::string out;
    q.try_pop(out);
    CHECK(out == "original");
}

TEST_CASE("FIFO ordering") {
    LockQueue<int, CAP> q;
    q.try_push(10);
    q.try_push(20);
    q.try_push(30);
    int v;
    q.try_pop(v); CHECK(v == 10);
    q.try_pop(v); CHECK(v == 20);
    q.try_pop(v); CHECK(v == 30);
    CHECK(q.isEmpty());
}

TEST_CASE("isFull when all slots are used") {
    LockQueue<int, CAP> q; // 4 usable slots
    for (int i = 0; i < 4; ++i) CHECK(q.try_push(i));
    CHECK(q.isFull());
    CHECK_FALSE(q.try_push(99)); // must reject
}

TEST_CASE("try_pop on empty queue returns false") {
    LockQueue<int, CAP> q;
    int v = -1;
    CHECK_FALSE(q.try_pop(v));
    CHECK(v == -1); // output parameter must be unchanged
}

TEST_CASE("wrap-around ring buffer") {
    LockQueue<int, 4> q; // 3 usable slots
    // Push/pop in three rounds to force the head and tail indices to wrap.
    for (int round = 0; round < 4; ++round) {
        for (int i = 0; i < 3; ++i) CHECK(q.try_push(i * 10));
        for (int i = 0; i < 3; ++i) {
            int v;
            CHECK(q.try_pop(v));
            CHECK(v == i * 10);
        }
    }
}

TEST_CASE("interleaved push and pop maintains FIFO and correct empty/full state") {
    LockQueue<int, 4> q; // 3 usable slots
    int v;
    q.try_push(1);
    q.try_push(2);
    q.try_pop(v); CHECK(v == 1);
    q.try_push(3);
    q.try_push(4); // now full
    CHECK(q.isFull());
    CHECK_FALSE(q.try_push(5));
    q.try_pop(v); CHECK(v == 2);
    q.try_pop(v); CHECK(v == 3);
    q.try_pop(v); CHECK(v == 4);
    CHECK(q.isEmpty());
}

// ---------------------------------------------------------------------------
// Multi-threaded: blocking push / pop
// ---------------------------------------------------------------------------

TEST_CASE("blocking push/pop - SPSC preserves order and delivers all items") {
    LockQueue<int, 8> q;
    constexpr int N = 1000;
    std::vector<int> results;
    results.reserve(N);

    std::thread producer([&] {
        for (int i = 0; i < N; ++i) q.push(i);
    });
    std::thread consumer([&] {
        for (int i = 0; i < N; ++i) {
            int v;
            q.pop(v);
            results.push_back(v);
        }
    });
    producer.join();
    consumer.join();

    REQUIRE((int)results.size() == N);
    for (int i = 0; i < N; ++i) CHECK(results[i] == i);
}

TEST_CASE("MPMC - all items are consumed exactly once") {
    LockQueue<int, 64> q;
    constexpr int NTHREADS  = 4;
    constexpr int PER_THREAD = 250;
    constexpr int TOTAL      = NTHREADS * PER_THREAD;

    std::atomic<int> consumed{0};
    std::vector<std::thread> producers, consumers;

    for (int t = 0; t < NTHREADS; ++t) {
        producers.emplace_back([&] {
            for (int i = 0; i < PER_THREAD; ++i) q.push(i);
        });
    }
    // Each consumer pops exactly PER_THREAD times; totals match push count.
    for (int t = 0; t < NTHREADS; ++t) {
        consumers.emplace_back([&] {
            for (int i = 0; i < PER_THREAD; ++i) {
                int v;
                q.pop(v);
                consumed.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& p : producers) p.join();
    for (auto& c : consumers) c.join();

    CHECK(consumed.load() == TOTAL);
}

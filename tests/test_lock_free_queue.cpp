#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "tests/doctest/doctest.h"

#include "queue_lock_free.hpp"
#include <string>
#include <thread>
#include <vector>

// Capacity=5 → 4 usable slots (one sentinel slot reserved).
static constexpr std::size_t CAP = 5;

// ---------------------------------------------------------------------------
// Single-threaded: state queries
// ---------------------------------------------------------------------------

TEST_CASE("isEmpty and isFull on a new queue") {
    SPSCQueueLockFree<int, CAP> q;
    CHECK(q.isEmpty());
    CHECK_FALSE(q.isFull());
}

// ---------------------------------------------------------------------------
// Single-threaded: try_push / try_pop
// ---------------------------------------------------------------------------

TEST_CASE("try_push (copy) and try_pop - basic round-trip") {
    SPSCQueueLockFree<int, CAP> q;
    int v = 0;
    CHECK(q.try_push(1));
    CHECK_FALSE(q.isEmpty());
    CHECK(q.try_pop(v));
    CHECK(v == 1);
    CHECK(q.isEmpty());
}

TEST_CASE("try_push (move) and try_pop - basic round-trip") {
    SPSCQueueLockFree<std::string, CAP> q;
    std::string s = "hello";
    CHECK(q.try_push(std::move(s)));
    std::string out;
    CHECK(q.try_pop(out));
    CHECK(out == "hello");
}

TEST_CASE("copy semantics - original value unchanged after try_push") {
    SPSCQueueLockFree<std::string, CAP> q;
    std::string s = "original";
    q.try_push(s);
    CHECK(s == "original");
    std::string out;
    q.try_pop(out);
    CHECK(out == "original");
}

TEST_CASE("FIFO ordering") {
    SPSCQueueLockFree<int, CAP> q;
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
    SPSCQueueLockFree<int, CAP> q; // 4 usable slots
    for (int i = 0; i < 4; ++i) CHECK(q.try_push(i));
    CHECK(q.isFull());
    CHECK_FALSE(q.try_push(99)); // must reject
}

TEST_CASE("try_pop on empty queue returns false") {
    SPSCQueueLockFree<int, CAP> q;
    int v = -1;
    CHECK_FALSE(q.try_pop(v));
    CHECK(v == -1); // output parameter must be unchanged
}

TEST_CASE("wrap-around ring buffer") {
    SPSCQueueLockFree<int, 4> q; // 3 usable slots
    // Push/pop in several rounds to force the head and tail indices to wrap.
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
    SPSCQueueLockFree<int, 4> q; // 3 usable slots
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
// Multi-threaded: SPSC concurrent correctness
// ---------------------------------------------------------------------------

TEST_CASE("SPSC concurrent - no data loss and correct order") {
    SPSCQueueLockFree<int, 128> q;
    constexpr int N = 10'000;
    std::vector<int> results;
    results.reserve(N);

    std::thread producer([&] {
        for (int i = 0; i < N; ++i) {
            while (!q.try_push(i)) { /* spin until space available */ }
        }
    });
    std::thread consumer([&] {
        while ((int)results.size() < N) {
            int v;
            if (q.try_pop(v)) results.push_back(v);
        }
    });
    producer.join();
    consumer.join();

    REQUIRE((int)results.size() == N);
    for (int i = 0; i < N; ++i) CHECK(results[i] == i);
}

TEST_CASE("SPSC concurrent - high throughput wrap-around") {
    // Uses a small buffer to force many wrap-arounds during the concurrent run.
    SPSCQueueLockFree<int, 8> q; // 7 usable slots
    constexpr int N = 5'000;
    std::vector<int> results;
    results.reserve(N);

    std::thread producer([&] {
        for (int i = 0; i < N; ++i) {
            while (!q.try_push(i)) {}
        }
    });
    std::thread consumer([&] {
        while ((int)results.size() < N) {
            int v;
            if (q.try_pop(v)) results.push_back(v);
        }
    });
    producer.join();
    consumer.join();

    REQUIRE((int)results.size() == N);
    for (int i = 0; i < N; ++i) CHECK(results[i] == i);
}

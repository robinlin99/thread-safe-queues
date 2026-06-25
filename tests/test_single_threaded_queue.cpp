#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "tests/doctest/doctest.h"

#include "single_threaded_queue.hpp"
#include <string>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_CASE("zero capacity throws invalid_argument") {
    CHECK_THROWS_AS(VectorQueue<int>(0), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// State queries
// ---------------------------------------------------------------------------

TEST_CASE("isEmpty on a new queue") {
    VectorQueue<int> q(4);
    CHECK(q.isEmpty());
    CHECK(q.size() == 0);
}

TEST_CASE("isEmpty and size after push") {
    VectorQueue<int> q(4);
    q.push(1);
    CHECK_FALSE(q.isEmpty());
    CHECK(q.size() == 1);
}

// ---------------------------------------------------------------------------
// front / back
// ---------------------------------------------------------------------------

TEST_CASE("front and back on empty queue return false") {
    VectorQueue<int> q(4);
    int v = -1;
    CHECK_FALSE(q.front(v));
    CHECK_FALSE(q.back(v));
    CHECK(v == -1); // output parameter must be unchanged
}

TEST_CASE("front and back with a single element") {
    VectorQueue<int> q(4);
    q.push(42);
    int f, b;
    CHECK(q.front(f)); CHECK(f == 42);
    CHECK(q.back(b));  CHECK(b == 42);
}

TEST_CASE("front and back with multiple elements") {
    VectorQueue<int> q(4);
    q.push(1);
    q.push(2);
    q.push(3);
    int f, b;
    CHECK(q.front(f)); CHECK(f == 1);
    CHECK(q.back(b));  CHECK(b == 3);
}

TEST_CASE("front and back do not remove elements") {
    VectorQueue<int> q(4);
    q.push(10);
    q.push(20);
    int v;
    q.front(v);
    q.back(v);
    CHECK(q.size() == 2);
}

// ---------------------------------------------------------------------------
// push / pop
// ---------------------------------------------------------------------------

TEST_CASE("push (copy) and pop - basic round-trip") {
    VectorQueue<int> q(4);
    int v = 0;
    CHECK(q.push(1));
    CHECK(q.pop(v));
    CHECK(v == 1);
    CHECK(q.isEmpty());
}

TEST_CASE("push (move) and pop - basic round-trip") {
    VectorQueue<std::string> q(4);
    std::string s = "hello";
    CHECK(q.push(std::move(s)));
    std::string out;
    CHECK(q.pop(out));
    CHECK(out == "hello");
}

TEST_CASE("copy push - original value unchanged") {
    VectorQueue<std::string> q(4);
    std::string s = "original";
    q.push(s);
    CHECK(s == "original");
    std::string out;
    q.pop(out);
    CHECK(out == "original");
}

TEST_CASE("pop on empty queue returns false") {
    VectorQueue<int> q(4);
    int v = -1;
    CHECK_FALSE(q.pop(v));
    CHECK(v == -1); // output parameter must be unchanged
}

TEST_CASE("FIFO ordering") {
    VectorQueue<int> q(8);
    q.push(10);
    q.push(20);
    q.push(30);
    int v;
    q.pop(v); CHECK(v == 10);
    q.pop(v); CHECK(v == 20);
    q.pop(v); CHECK(v == 30);
    CHECK(q.isEmpty());
}

TEST_CASE("interleaved push and pop maintains FIFO") {
    VectorQueue<int> q(4);
    int v;
    q.push(1);
    q.push(2);
    q.pop(v); CHECK(v == 1);
    q.push(3);
    q.push(4);
    q.pop(v); CHECK(v == 2);
    q.pop(v); CHECK(v == 3);
    q.pop(v); CHECK(v == 4);
    CHECK(q.isEmpty());
}

// ---------------------------------------------------------------------------
// Growth / wrap-around
// ---------------------------------------------------------------------------

TEST_CASE("grow - push beyond initial capacity") {
    VectorQueue<int> q(2);
    // Push more than the initial capacity to trigger grow().
    for (int i = 0; i < 8; ++i) q.push(i);
    CHECK(q.size() == 8);
    for (int i = 0; i < 8; ++i) {
        int v;
        q.pop(v);
        CHECK(v == i);
    }
    CHECK(q.isEmpty());
}

TEST_CASE("wrap-around ring buffer") {
    VectorQueue<int> q(4);
    // Push/pop in rounds to force head and tail indices to wrap.
    for (int round = 0; round < 4; ++round) {
        for (int i = 0; i < 4; ++i) q.push(i * 10);
        for (int i = 0; i < 4; ++i) {
            int v;
            q.pop(v);
            CHECK(v == i * 10);
        }
    }
}

TEST_CASE("grow preserves element order when head is not at index 0") {
    VectorQueue<int> q(4);
    // Advance head by popping a few elements first, then fill to trigger grow.
    q.push(0); q.push(1); q.push(2); q.push(3);
    int v;
    q.pop(v); q.pop(v); // head is now at index 2
    q.push(4); q.push(5); // tail wraps; next push will need to grow
    q.push(6);             // triggers grow with a wrapped buffer

    for (int expected = 2; expected <= 6; ++expected) {
        q.pop(v);
        CHECK(v == expected);
    }
    CHECK(q.isEmpty());
}

// ---------------------------------------------------------------------------
// clear
// ---------------------------------------------------------------------------

TEST_CASE("clear empties the queue") {
    VectorQueue<int> q(4);
    q.push(1); q.push(2); q.push(3);
    q.clear();
    CHECK(q.isEmpty());
    CHECK(q.size() == 0);
}

TEST_CASE("clear allows reuse after clearing") {
    VectorQueue<int> q(4);
    q.push(1); q.push(2);
    q.clear();
    q.push(10); q.push(20);
    int v;
    q.pop(v); CHECK(v == 10);
    q.pop(v); CHECK(v == 20);
    CHECK(q.isEmpty());
}

// ---------------------------------------------------------------------------
// Size tracking
// ---------------------------------------------------------------------------

TEST_CASE("size tracks correctly across push and pop") {
    VectorQueue<int> q(8);
    for (int i = 0; i < 5; ++i) q.push(i);
    CHECK(q.size() == 5);
    int v;
    q.pop(v); CHECK(q.size() == 4);
    q.pop(v); CHECK(q.size() == 3);
    q.push(99); CHECK(q.size() == 4);
}

/**
 * @file sample_single_threaded_queue.cpp
 * @brief Sample application demonstrating VectorQueue's dynamic growth.
 *
 * Starts with a small initial capacity and pushes enough items to trigger
 * multiple grow() cycles. Prints capacity checkpoints as the buffer doubles,
 * then drains the queue verifying FIFO order.
 */

#include "single_threaded_queue.hpp"

#include <cstdint>
#include <iostream>

/// Initial queue capacity — intentionally small to provoke several grows.
static constexpr std::size_t INITIAL_CAPACITY = 4;

/// Total items pushed — enough to double the buffer multiple times.
static constexpr std::size_t NUM_ITEMS = 64;

int main() {
    VectorQueue<std::uint64_t> q(INITIAL_CAPACITY);

    std::cout << "=== VectorQueue Growth Demo ===\n";
    std::cout << "Initial capacity: " << INITIAL_CAPACITY << "\n\n";

    // Push items one at a time; report each growth event.
    std::size_t last_size = INITIAL_CAPACITY;
    for (std::size_t i = 0; i < NUM_ITEMS; ++i) {
        q.push(i);

        // Detect a grow by checking whether the logical size just crossed a
        // power-of-two boundary (VectorQueue doubles on each grow).
        std::size_t current_size = q.size();
        if (current_size > last_size && (current_size & (current_size - 1)) == 0) {
            std::cout << "  [grow] buffer doubled — now holding "
                      << current_size << " elements\n";
            last_size = current_size;
        }
    }

    std::cout << "\nPushed " << NUM_ITEMS << " items.\n";

    // Peek at front and back before draining.
    std::uint64_t front_val, back_val;
    q.front(front_val);
    q.back(back_val);
    std::cout << "Front: " << front_val << "  Back: " << back_val << "\n\n";

    // Drain the queue and verify FIFO ordering.
    std::cout << "Draining queue...\n";
    bool order_ok = true;
    for (std::size_t i = 0; i < NUM_ITEMS; ++i) {
        std::uint64_t val;
        q.pop(val);
        if (val != static_cast<std::uint64_t>(i)) {
            order_ok = false;
            std::cout << "  Order violation at position " << i
                      << ": expected " << i << ", got " << val << "\n";
        }
    }

    std::cout << "Queue empty: " << (q.isEmpty() ? "yes" : "no") << "\n";
    std::cout << "FIFO order:  " << (order_ok ? "correct" : "VIOLATED") << "\n";

    return order_ok ? 0 : 1;
}

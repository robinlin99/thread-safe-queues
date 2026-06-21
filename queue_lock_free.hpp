/**
 * @file queue_lock_free.hpp
 * @brief Lock-free bounded SPSC (single-producer / single-consumer) queue.
 *
 * Implements a fixed-capacity circular buffer using atomic load/store with
 * acquire/release memory ordering. No mutexes or system calls are involved;
 * synchronization relies entirely on the cache-coherence protocol.
 *
 * Constraints:
 * - Exactly ONE producer thread may call try_push().
 * - Exactly ONE consumer thread may call try_pop().
 * - Violating the SPSC contract is undefined behavior.
 *
 * The head_ and tail_ indices are aligned to separate cache lines (64 bytes)
 * to eliminate false sharing between producer and consumer cores.
 *
 * @tparam T        Element type stored in the queue.
 * @tparam Capacity Total number of slots in the ring buffer. Usable capacity
 *                  is Capacity - 1 (one slot reserved as a sentinel to
 *                  distinguish full from empty).
 */

#include <cstddef>
#include <array>
#include <atomic>
#include <utility>


template <typename T, std::size_t Capacity>
class SPSCQueueLockFree {
public:
    /**
     * @brief Checks whether the queue is empty.
     * @return true if the queue contains no elements.
     *
     * Uses acquire loads on both indices. The result is a point-in-time
     * snapshot and may be stale immediately after returning.
     */
    bool isEmpty() const {
        return tail_.load(std::memory_order_acquire) == head_.load(std::memory_order_acquire);
    }

    /**
     * @brief Checks whether the queue is full.
     * @return true if the queue cannot accept another element.
     *
     * Uses acquire loads on both indices. The result is a point-in-time
     * snapshot and may be stale immediately after returning.
     */
    bool isFull() const {
        return (tail_.load(std::memory_order_acquire) + 1) % Capacity == head_.load(std::memory_order_acquire);
    }

    /**
     * @brief Attempts to enqueue an element (move semantics).
     * @param value Rvalue reference to the element to enqueue.
     * @return true if the element was enqueued, false if the queue is full.
     *
     * Non-blocking. The producer reads tail_ with relaxed ordering (only this
     * thread writes it) and head_ with acquire ordering to observe the
     * consumer's progress. On success, the tail_ store uses release ordering
     * to make the written element visible to the consumer.
     */
    bool try_push(T&& value) {
        std::size_t tail = tail_.load(std::memory_order_relaxed);
        std::size_t next_tail = (tail + 1) % Capacity;
        std::size_t head = head_.load(std::memory_order_acquire);
        if (next_tail == head) {
            return false;
        }

        array_[tail] = std::move(value);
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    /**
     * @brief Attempts to enqueue an element (copy semantics).
     * @param value Const reference to the element to enqueue.
     * @return true if the element was enqueued, false if the queue is full.
     *
     * Same ordering guarantees as the move overload.
     */
    bool try_push(const T& value) {
        std::size_t tail = tail_.load(std::memory_order_relaxed);
        std::size_t next_tail = (tail + 1) % Capacity;
        std::size_t head = head_.load(std::memory_order_acquire);
        if (next_tail == head) {
            return false;
        }

        array_[tail] = value;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    /**
     * @brief Attempts to dequeue an element.
     * @param[out] value Receives the dequeued element via move.
     * @return true if an element was dequeued, false if the queue is empty.
     *
     * Non-blocking. The consumer reads head_ with relaxed ordering (only this
     * thread writes it) and tail_ with acquire ordering to observe the
     * producer's progress. On success, the head_ store uses release ordering
     * to signal the producer that the slot is now free.
     */
    bool try_pop(T& value) {
        std::size_t head = head_.load(std::memory_order_relaxed);
        std::size_t next_head = (head + 1) % Capacity;
        std::size_t tail = tail_.load(std::memory_order_acquire);
        if (head == tail) {
            return false;
        }

        value = std::move(array_[head]);
        head_.store(next_head, std::memory_order_release);
        return true;
    }

private:
    /// Producer-owned index: points to the next slot to write.
    /// Aligned to a 64-byte cache line to avoid false sharing with head_.
    alignas(64) std::atomic<std::size_t> tail_ = 0;

    /// Consumer-owned index: points to the next slot to read.
    /// Aligned to a separate 64-byte cache line from tail_.
    alignas(64) std::atomic<std::size_t> head_ = 0;

    /// Fixed-size ring buffer storage.
    std::array<T, Capacity> array_;
};
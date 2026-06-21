/**
 * @file queue_lock.hpp
 * @brief Thread-safe bounded queue using mutex and condition variables.
 *
 * Implements a fixed-capacity circular buffer (ring buffer) protected by a
 * single mutex. Condition variables provide efficient blocking when the queue
 * is full (producers wait) or empty (consumers wait).
 *
 * Safe for multiple producers and multiple consumers concurrently.
 *
 * @tparam T        Element type stored in the queue.
 * @tparam Capacity Total number of slots in the ring buffer. Usable capacity
 *                  is Capacity - 1 (one slot reserved as a sentinel to
 *                  distinguish full from empty).
 */

#include <cstddef>
#include <mutex>
#include <condition_variable>
#include <array>
#include <utility>

template<typename T, std::size_t Capacity>
class LockQueue {
public:
    /**
     * @brief Checks whether the queue is empty.
     * @return true if the queue contains no elements.
     *
     * Acquires the mutex; result is a snapshot that may be stale by the time
     * the caller acts on it.
     */
    bool isEmpty() const {
        std::unique_lock lock(mtx_);
        return head_ == tail_;
    }

    /**
     * @brief Checks whether the queue is full.
     * @return true if the queue cannot accept another element.
     *
     * Acquires the mutex; result is a snapshot that may be stale by the time
     * the caller acts on it.
     */
    bool isFull() const {
        std::unique_lock lock(mtx_);
        return (tail_ + 1) % Capacity == head_;
    }

    /**
     * @brief Blocking push (move semantics).
     * @param value Rvalue reference to the element to enqueue.
     * @return Always returns true (blocks until space is available).
     *
     * Waits on the notFull_ condition variable if the queue is full.
     * Notifies one waiting consumer after insertion.
     */
    bool push(T&& value) {
        std::unique_lock lock(mtx_);
        notFull_.wait(lock, [&] { return (tail_ + 1) % Capacity != head_; });

        array_[tail_] = std::move(value);
        tail_ = (tail_ + 1) % Capacity;
        lock.unlock();
        notEmpty_.notify_one();
        return true;
    }

    /**
     * @brief Blocking push (copy semantics).
     * @param value Const reference to the element to enqueue.
     * @return Always returns true (blocks until space is available).
     *
     * Waits on the notFull_ condition variable if the queue is full.
     * Notifies one waiting consumer after insertion.
     */
    bool push(const T& value) {
        std::unique_lock lock(mtx_);
        notFull_.wait(lock, [&] { return (tail_ + 1) % Capacity != head_; });

        array_[tail_] = value;
        tail_ = (tail_ + 1) % Capacity;
        lock.unlock();
        notEmpty_.notify_one();
        return true;
    }

    /**
     * @brief Blocking pop.
     * @param[out] value Receives the dequeued element via move.
     * @return Always returns true (blocks until an element is available).
     *
     * Waits on the notEmpty_ condition variable if the queue is empty.
     * Notifies one waiting producer after removal.
     */
    bool pop(T& value) {
        std::unique_lock lock(mtx_);
        notEmpty_.wait(lock, [&] { return head_ != tail_; });
        
        value = std::move(array_[head_]);
        head_ = (head_ + 1) % Capacity;
        lock.unlock();
        notFull_.notify_one();
        return true;
    }

    /**
     * @brief Non-blocking push (move semantics).
     * @param value Rvalue reference to the element to enqueue.
     * @return true if the element was enqueued, false if the queue was full.
     *
     * Does not block — returns immediately if the queue is full.
     * Notifies one waiting consumer on success.
     */
    bool try_push(T&& value) {
        std::unique_lock lock(mtx_);
        if ((tail_ + 1) % Capacity == head_) {
            return false;
        }

        array_[tail_] = std::move(value);
        tail_ = (tail_ + 1) % Capacity;
        lock.unlock();
        notEmpty_.notify_one();
        return true;
    }

    /**
     * @brief Non-blocking push (copy semantics).
     * @param value Const reference to the element to enqueue.
     * @return true if the element was enqueued, false if the queue was full.
     *
     * Does not block — returns immediately if the queue is full.
     * Notifies one waiting consumer on success.
     */
    bool try_push(const T& value) {
        std::unique_lock lock(mtx_);
        if ((tail_ + 1) % Capacity == head_) {
            return false;
        }

        array_[tail_] = value;
        tail_ = (tail_ + 1) % Capacity;
        lock.unlock();
        notEmpty_.notify_one();
        return true;
    }

    /**
     * @brief Non-blocking pop.
     * @param[out] value Receives the dequeued element via move.
     * @return true if an element was dequeued, false if the queue was empty.
     *
     * Does not block — returns immediately if the queue is empty.
     * Notifies one waiting producer on success.
     */
    bool try_pop(T& value) {
        std::unique_lock lock(mtx_);
        if (head_ == tail_) {
            return false;
        }

        value = std::move(array_[head_]);
        head_ = (head_ + 1) % Capacity;
        lock.unlock();
        notFull_.notify_one();
        return true;
    }

private:
    size_t head_ = 0;  ///< Index of the next element to dequeue.
    size_t tail_ = 0;  ///< Index of the next free slot for enqueue.

    mutable std::mutex mtx_;              ///< Protects all shared state.
    std::condition_variable notEmpty_;     ///< Signaled when queue becomes non-empty.
    std::condition_variable notFull_;      ///< Signaled when queue becomes non-full.
    std::array<T, Capacity> array_;       ///< Fixed-size ring buffer storage.
};
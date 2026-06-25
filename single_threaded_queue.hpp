#include <cstddef>
#include <stdexcept>
#include <vector>
#include <utility>

/**
 * @brief A circular-buffer queue backed by a dynamically-resizing vector.
 *
 * Provides amortized O(1) push/pop and O(1) front/back access.
 * Not thread-safe — intended for single-threaded use only.
 *
 * @tparam T Type of elements stored in the queue.
 */
template <typename T>
class VectorQueue {
public:
    /**
     * @brief Constructs a queue with the given initial capacity.
     * @param capacity Initial capacity; must be greater than 0.
     * @throws std::invalid_argument if capacity is 0.
     */
    VectorQueue(std::size_t capacity) : vec_(capacity), head_(0), tail_(0), size_(0) {
        if (capacity == 0)
            throw std::invalid_argument("VectorQueue: capacity must be greater than 0");
    }

    /**
     * @brief Returns true if the queue contains no elements.
     */
    bool isEmpty() const {
        return size_ == 0;
    }

    /**
     * @brief Returns the number of elements currently in the queue.
     */
    std::size_t size() const {
        return size_;
    }

    /**
     * @brief Reads the front element without removing it.
     * @param val Output parameter set to the front element if the queue is non-empty.
     * @return true if the queue was non-empty and val was set; false otherwise.
     */
    bool front(T& val) const {
        if (isEmpty())
            return false;
        
        val = vec_[head_];
        return true;
    }

    /**
     * @brief Reads the back element without removing it.
     * @param val Output parameter set to the back element if the queue is non-empty.
     * @return true if the queue was non-empty and val was set; false otherwise.
     */
    bool back(T& val) const {
        if (isEmpty())
            return false;

        val = vec_[(tail_ - 1 + vec_.size()) % vec_.size()];
        
        return true;
    }

    /**
     * @brief Pushes a copy of val onto the back of the queue.
     *        Grows the internal buffer if at capacity.
     * @param val Value to enqueue.
     * @return Always returns true.
     */
    bool push(const T& val) {
        if (size_ == vec_.size()) {
            grow();
        }

        vec_[tail_] = val;
        tail_ = (tail_ + 1) % vec_.size();
        size_++;
        return true;
    }

    /**
     * @brief Pushes val onto the back of the queue by move.
     *        Grows the internal buffer if at capacity.
     * @param val Value to move-enqueue.
     * @return Always returns true.
     */
    bool push(T&& val) {
        if (size_ == vec_.size()) {
            grow();
        }

        vec_[tail_] = std::move(val);
        tail_ = (tail_ + 1) % vec_.size();
        size_++;
        return true;
    }

    /**
     * @brief Removes all elements from the queue without releasing the buffer.
     */
    void clear() {
        head_ = 0;
        tail_ = 0;
        size_ = 0;
    }

    /**
     * @brief Removes the front element and moves it into val.
     * @param val Output parameter set to the popped element if the queue is non-empty.
     * @return true if an element was removed; false if the queue was empty.
     */
    bool pop(T& val) {
        if (isEmpty()) {
            return false;
        }

        val = std::move(vec_[head_]);
        head_ = (head_ + 1) % vec_.size();
        size_--;

        return true;
    }

private:
    /**
     * @brief Doubles the internal buffer capacity and repacks elements in order.
     *        O(N) time complexity.
     */
    void grow() {
        std::size_t old_cap = vec_.size();
        std::size_t new_cap = 2 * old_cap;
        std::vector<T> new_vec(new_cap);

        for (size_t i = 0; i < size_; ++i) {
            new_vec[i] = vec_[(head_ + i) % old_cap];
        }

        vec_ = std::move(new_vec);
        head_ = 0;
        tail_ = size_;
    }

    std::vector<T> vec_;
    std::size_t head_;
    std::size_t tail_;
    std::size_t size_;
};

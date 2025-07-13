/*======================================================================*
 |  ring_queue.hpp – lock-free multi-producer / single-consumer queue   |
 *======================================================================*/
#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <type_traits>

namespace vis::concur {

template <typename T, std::size_t CapacityPow2>
class alignas(64) RingQueue {
    static_assert((CapacityPow2 & (CapacityPow2 - 1)) == 0,
                  "Capacity must be a power of two");
public:
    RingQueue() : head_(0), tail_(0) {
        static_assert(std::is_trivially_copyable_v<T>,
                      "RingQueue requires trivially-copyable payload");
        buffer_ = static_cast<T*>(::operator new[](CapacityPow2 * sizeof(T),
                                                  std::align_val_t{64}));
    }

    ~RingQueue() { ::operator delete[](buffer_, std::align_val_t{64}); }

    /// Returns false if queue is full.
    bool push(const T& item) noexcept {
        auto head = head_.load(std::memory_order_relaxed);
        auto next = head + 1;
        if (next - tail_.load(std::memory_order_acquire) > CapacityPow2)
            return false; // full
        buffer_[head & (CapacityPow2 - 1)] = item;
        head_.store(next, std::memory_order_release);
        return true;
    }

    /// Returns false if queue is empty.
    bool pop(T& out) noexcept {
        auto tail = tail_.load(std::memory_order_relaxed);
        if (head_.load(std::memory_order_acquire) == tail) return false; // empty
        out = buffer_[tail & (CapacityPow2 - 1)];
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

private:
    T*                   buffer_;
    std::atomic<uint64_t> head_;
    std::atomic<uint64_t> tail_;
};

} // namespace vis::concur

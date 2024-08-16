//
// Created by ABDERRAHIM ZEBIRI on 2024-08-10.
//
//
// Created by ABDERRAHIM ZEBIRI on 2024-08-10.
//

#ifndef LOW_LATENCY_TRADING_APP_LOCK_FREE_QUEUE_H
#define LOW_LATENCY_TRADING_APP_LOCK_FREE_QUEUE_H

#include <atomic>
#include <vector>
#include <cstddef>
#include <optional>

#include "debug_assertion.h"


namespace utils {

template <typename T>
class LFQueue {
  public:
    explicit LFQueue(std::size_t size)
        : queue_(size){}

    LFQueue() = delete;
    LFQueue(const LFQueue&) = delete;
    LFQueue(LFQueue&&) = delete;
    LFQueue& operator=(const LFQueue&) = delete;
    LFQueue& operator=(LFQueue&&) = delete;


    bool push(const T& value) noexcept {
        auto* slot = getNextToWrite();
        if (!slot) [[unlikely]] return false;
        *slot = value;
        updateWriteIndex();
        return true;
    }

    std::optional<T> pop() noexcept {
        const T* elem = getNextToRead();
        if (!elem) [[unlikely]] return std::nullopt;
        T value = *elem;
        updateReadIndex();
        return value;
    }

    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return numElements_.load(std::memory_order_relaxed);
    }

  private:

    [[nodiscard]] auto getNextToRead() const noexcept -> const T* {
        return numElements_ > 0 ? &queue_[nextIndexToRead_] : nullptr;
    }

    [[nodiscard]] auto getNextToWrite() noexcept -> T* {
        return numElements_ < queue_.size() ? &queue_[nextIndexToWrite_] : nullptr;
    }

    auto updateWriteIndex() noexcept -> void {
        nextIndexToWrite_ = (nextIndexToWrite_ + 1) % queue_.size();
        numElements_.fetch_add(1, std::memory_order_relaxed);
    }

    auto updateReadIndex() noexcept -> void {
        assertCondition(numElements_ > 0, "No elements to read.");
        nextIndexToRead_ = (nextIndexToRead_ + 1) % queue_.size();
        numElements_.fetch_sub(1, std::memory_order_relaxed);
    }

    std::atomic<std::size_t> nextIndexToRead_{0};
    std::atomic<std::size_t> nextIndexToWrite_{0};
    std::atomic<std::size_t> numElements_{0};
    std::vector<T> queue_;
};

} // namespace utils

#endif // LOW_LATENCY_TRADING_APP_LOCK_FREE_QUEUE_H
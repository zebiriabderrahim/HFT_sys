//
// Created by ABDERRAHIM ZEBIRI on 2024-08-10.
//

#ifndef LOW_LATENCY_TRADING_APP_LOOK_FREE_QUEUE_H
#define LOW_LATENCY_TRADING_APP_LOOK_FREE_QUEUE_H

#include <atomic>
#include <utility>
#include <vector>
#include <string>
#include <thread>

#include "debug_assertion.h"


namespace utils {

template <typename T>
class LFQueue {
        public:
        explicit LFQueue(std::size_t size):queue_(size,T()){};

        LFQueue() = delete;
        LFQueue(const LFQueue &) = delete;
        LFQueue(LFQueue &&) = delete;

        LFQueue &operator=(const LFQueue &) = delete;
        LFQueue &operator=(LFQueue &&) = delete;

        auto getNextIndexToRead() const noexcept -> const T*{
            return &queue_[nextIndexToRead_];
        }
        auto getNextIndexToWrite_() const noexcept -> const T*{
            return size() ?  &queue_[nextIndexToWrite_] : nullptr ;
        }

        auto updateWriteIndex() noexcept -> void {
            nextIndexToWrite_ = (nextIndexToWrite_ + 1) % queue_.size();
            numElements_++;
        }
        auto updateReadIndex() noexcept {
            nextIndexToRead_ = (nextIndexToRead_ + 1) % queue_.size();
            assertCondition(numElements_ != 0, "Read an invalid element in:");
            numElements_--;
        }

        auto size() const noexcept {
            return numElements_.load();
        }

      private:
        std::atomic<std::size_t> nextIndexToRead_{0};
        std::atomic<std::size_t> nextIndexToWrite_{0};
        std::atomic<std::size_t> numElements_{0};
        std::vector<T> queue_;


};

} // namespace utils

#endif // LOW_LATENCY_TRADING_APP_LOOK_FREE_QUEUE_H

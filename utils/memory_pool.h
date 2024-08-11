//
// Created by ABDERRAHIM ZEBIRI on 2024-08-08.
//

#ifndef LOW_LATENCY_TRADING_APP_MEMORY_POOL_H
#define LOW_LATENCY_TRADING_APP_MEMORY_POOL_H

#include <format>
#include <vector>

#include "debug_assertion.h"

namespace utils {
template <typename T> class MemoryPool {

  public:
    explicit MemoryPool(std::size_t size) : memoryBlocks_(size, {T(), true}) {
        assertCondition(reinterpret_cast<const MemoryBlock *>(&(memoryBlocks_[0].object)) == &(memoryBlocks_[0]),
                        "T object should be first member of MemoryBlock.");
    }

    MemoryPool() = delete;
    MemoryPool(const MemoryPool &) = delete;
    MemoryPool(MemoryPool &&) = delete;

    MemoryPool &operator=(const MemoryPool &) = delete;
    MemoryPool &operator=(MemoryPool &&) = delete;

    template<typename... Args>
    T *allocate(Args &&...args) {
        auto memoryBlock = &(memoryBlocks_[nextFreeIndex_]);

        assertCondition(memoryBlock->isFree,
                        std::format("Expected free MemoryBlock at index:{}", std::to_string(nextFreeIndex_)));

        T *ret = &(memoryBlock->object);
        ret = new (ret) T(std::forward<Args>(args)...); // placement new.
        memoryBlocks_[nextFreeIndex_].isFree = false;

        updateNextFreeIndex();
        return std::launder(ret);
    }

    void deallocate(const T* elem) noexcept {
        const auto * elemPtr = reinterpret_cast<const MemoryBlock*>(elem);
        auto elemIndex = static_cast<std::size_t>(elemPtr - &(memoryBlocks_[0]));

        assertCondition(elemIndex < memoryBlocks_.size(),"Invalid element index.");
        assertCondition(!memoryBlocks_[elemIndex].isFree,
                        std::format("Expected in-use MemoryBlock at index:{}", std::to_string(elemIndex)));

        memoryBlocks_[elemIndex].isFree = true;
    }


  private:
     void updateNextFreeIndex() noexcept {
        const auto initialFreeIndex = nextFreeIndex_;
        while (!memoryBlocks_[nextFreeIndex_].isFree) {
            ++nextFreeIndex_;
            if (nextFreeIndex_ == memoryBlocks_.size()) [[unlikely]] {
                nextFreeIndex_ = 0;
            }
            if (initialFreeIndex == nextFreeIndex_ ) [[unlikely]] {
                assertCondition(initialFreeIndex != nextFreeIndex_, "Memory Pool out of space.");
            }
        }
    }

    struct MemoryBlock {
        T object;
        bool isFree = true;
    };

    std::vector<MemoryBlock> memoryBlocks_;
    std::size_t nextFreeIndex_{0};
};

} // namespace utils

#endif // LOW_LATENCY_TRADING_APP_MEMORY_POOL_H

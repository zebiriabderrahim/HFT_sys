//
// Created by ABDERRAHIM ZEBIRI on 2024-08-08.
//
#ifndef LOW_LATENCY_TRADING_APP_MEMORY_POOL_H
#define LOW_LATENCY_TRADING_APP_MEMORY_POOL_H

#include <format>
#include <vector>
#include <memory>
#include "debug_assertion.h"

namespace utils {
template <typename T>
class MemoryPool {
  public:
    explicit MemoryPool(std::size_t size) : memoryBlocks_(size) {
        ASSERT_CONDITION(reinterpret_cast<const MemoryBlock *>(&(memoryBlocks_[0].storage)) == &(memoryBlocks_[0]),
                         "Storage should be first member of MemoryBlock.");
    }

    MemoryPool() = delete;
    MemoryPool(const MemoryPool &) = delete;
    MemoryPool(MemoryPool &&) = delete;
    MemoryPool &operator=(const MemoryPool &) = delete;
    MemoryPool &operator=(MemoryPool &&) = delete;

    template<typename... Args>
    T *allocate(Args &&...args) {
        auto memoryBlock = &(memoryBlocks_[nextFreeIndex_]);

        ASSERT_CONDITION(memoryBlock->isFree, "Expected free MemoryBlock at index:{}", std::to_string(nextFreeIndex_));

        auto *ret = reinterpret_cast<T*>(&(memoryBlock->storage));
        new (ret) T(std::forward<Args>(args)...); // placement new
        memoryBlocks_[nextFreeIndex_].isFree = false;

        updateNextFreeIndex();
        return ret;
    }

    void deallocate(T* elem) noexcept {
        const auto * elemPtr = reinterpret_cast<const MemoryBlock*>(elem);
        auto elemIndex = static_cast<std::size_t>(elemPtr - &(memoryBlocks_[0]));

        ASSERT_CONDITION(elemIndex < memoryBlocks_.size(), "Invalid element index.");
        ASSERT_CONDITION(!memoryBlocks_[elemIndex].isFree, "Expected in-use MemoryBlock at index:{}", std::to_string(elemIndex));

        elem->~T(); // Call destructor
        memoryBlocks_[elemIndex].isFree = true;
    }

  private:
    void updateNextFreeIndex() noexcept {
        const auto initialFreeIndex = nextFreeIndex_;
        do {
            ++nextFreeIndex_;
            if (nextFreeIndex_ == memoryBlocks_.size()) {
                nextFreeIndex_ = 0;
            }
            if (memoryBlocks_[nextFreeIndex_].isFree) {
                return;
            }
        } while (initialFreeIndex != nextFreeIndex_);
        ASSERT_CONDITION(false, "Memory Pool out of space.");
    }

    struct MemoryBlock {
        std::aligned_storage_t<sizeof(T), alignof(T)> storage;
        bool isFree = true;
    };

    std::vector<MemoryBlock> memoryBlocks_;
    std::size_t nextFreeIndex_{0};
};

} // namespace utils

#endif // LOW_LATENCY_TRADING_APP_MEMORY_POOL_H
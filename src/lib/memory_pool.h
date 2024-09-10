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
/**
 * @class MemoryPool
 * @brief A fixed-size memory pool for efficient allocation and deallocation of objects of type T.
 *
 * This class provides a memory pool implementation that pre-allocates a fixed number of memory blocks
 * for objects of type T. It allows for fast allocation and deallocation of objects without the overhead
 * of dynamic memory allocation.
 *
 * @tparam T The type of objects to be stored in the memory pool.
 */
template <typename T>
class MemoryPool {
  public:
    /**
     * @brief Constructs a MemoryPool with a specified number of memory blocks.
     *
     * @param size The number of memory blocks to pre-allocate..
     */
    explicit MemoryPool(std::size_t size) noexcept: memoryBlocks_(size) {
        ASSERT_CONDITION(reinterpret_cast<const MemoryBlock *>(&(memoryBlocks_[0].storage)) == &(memoryBlocks_[0]),
                         "Storage should be first member of MemoryBlock.");
    }

    // Deleted copy and move constructors and assignment operators
    MemoryPool() = delete;
    MemoryPool(const MemoryPool &) = delete;
    MemoryPool(MemoryPool &&) = delete;
    MemoryPool &operator=(const MemoryPool &) = delete;
    MemoryPool &operator=(MemoryPool &&) = delete;

    /**
     * @brief Allocates and constructs an object of type T in the memory pool.
     *
     * @tparam Args Types of the arguments to forward to T's constructor.
     * @param args Arguments to forward to T's constructor.
     * @return A pointer to the newly constructed object.
     */
    template<typename... Args>
    T *allocate(Args &&...args) noexcept {
         auto memoryBlock = &(memoryBlocks_[nextFreeIndex_]);

         ASSERT_CONDITION(memoryBlock->isFree, "Expected free MemoryBlock at index:{}", std::to_string(nextFreeIndex_));

         auto *ret = reinterpret_cast<T*>(&(memoryBlock->storage));
         new (ret) T(std::forward<Args>(args)...); // placement new
         memoryBlocks_[nextFreeIndex_].isFree = false;

         updateNextFreeIndex();
         return ret;
    }

    /**
     * @brief Destroys and deallocates an object previously allocated by this memory pool.
     *
     * @param elem A pointer to the object to deallocate.
     */
    auto deallocate(T* elem) noexcept -> void {
        const auto * elemPtr = reinterpret_cast<const MemoryBlock*>(elem);
        auto elemIndex = static_cast<std::size_t>(elemPtr - &(memoryBlocks_[0]));

        ASSERT_CONDITION(elemIndex < memoryBlocks_.size(), "Invalid element index.");
        ASSERT_CONDITION(!memoryBlocks_[elemIndex].isFree, "Expected in-use MemoryBlock at index:{}", std::to_string(elemIndex));

        elem->~T(); // Call destructor
        memoryBlocks_[elemIndex].isFree = true;
    }

  private:
    /**
     * @brief Updates the index of the next free memory block.
     * */
    auto updateNextFreeIndex() noexcept -> void {
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

    /**
     * @struct MemoryBlock
     * @brief Represents a single block of memory in the pool.
     */
    struct MemoryBlock {
        std::aligned_storage_t<sizeof(T), alignof(T)> storage; ///< Storage for an object of type T.
        bool isFree = true; ///< Indicates whether this block is currently free.
    };

    std::vector<MemoryBlock> memoryBlocks_; ///< The pre-allocated memory blocks.
    std::size_t nextFreeIndex_{0}; ///< Index of the next free memory block.
};

} // namespace lib

#endif // LOW_LATENCY_TRADING_APP_MEMORY_POOL_H
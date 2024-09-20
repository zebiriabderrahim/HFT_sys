#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <vector>
#include <cstddef>
#include <type_traits>
#include <new>
#include "assertion.h" // Make sure this header is available in your project

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
     * @param size The number of memory blocks to pre-allocate.
     */
    explicit MemoryPool(std::size_t size) noexcept
        : memoryBlocks_(size), freeBlocksCount_(size), lastFreedIndex_(0) {
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
     * @return A pointer to the newly constructed object, or nullptr if the pool is full.
     */
    template<typename... Args>
    T *allocate(Args &&...args) noexcept {
        if (freeBlocksCount_ == 0) {
            return nullptr;
        }

        auto memoryBlock = &(memoryBlocks_[nextFreeIndex_]);
        ASSERT_CONDITION(memoryBlock->isFree, "Expected free MemoryBlock at index:{}", std::to_string(nextFreeIndex_));

        auto *ret = reinterpret_cast<T*>(&(memoryBlock->storage));
        new (ret) T(std::forward<Args>(args)...); // placement new
        memoryBlock->isFree = false;
        --freeBlocksCount_;

        if (freeBlocksCount_ > 0) {
            updateNextFreeIndex();
        }
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
        ++freeBlocksCount_;

        lastFreedIndex_ = elemIndex;
        nextFreeIndex_ = lastFreedIndex_; // Set nextFreeIndex_ to the most recently freed block
    }

    /**
     * @brief Returns the number of free blocks in the pool.
     *
     * @return The number of free blocks.
     */
    [[nodiscard]] auto getFreeBlocksCount() const noexcept -> std::size_t {
        return freeBlocksCount_;
    }

    /**
     * @brief Returns the total number of blocks in the pool.
     *
     * @return The total number of blocks.
     */
    [[nodiscard]] auto getTotalBlocksCount() const noexcept -> std::size_t {
        return memoryBlocks_.size();
    }

  private:
    /**
     * @brief Updates the index of the next free memory block.
     *
     * This method searches for the next free block in the pool, starting from the current
     * nextFreeIndex_ and wrapping around if necessary. If no free block is found, it triggers
     * an assertion failure.
     */
    auto updateNextFreeIndex() noexcept -> void {
        if (memoryBlocks_[nextFreeIndex_].isFree) {
            return; // Current index is already free, no need to update
        }

        std::size_t startIndex = nextFreeIndex_;
        do {
            nextFreeIndex_ = (nextFreeIndex_ + 1) % memoryBlocks_.size();
            if (memoryBlocks_[nextFreeIndex_].isFree) {
                return;
            }
        } while (nextFreeIndex_ != startIndex);

        // If we've gone through all blocks and none are free
        ASSERT_CONDITION(false, "No free blocks found in Memory Pool.");
    }

    /**
     * @struct MemoryBlock
     * @brief Represents a single block of memory in the pool.
     *
     * Each MemoryBlock contains storage for an object of type T and a flag indicating
     * whether the block is currently free or in use.
     */
    struct MemoryBlock {
        std::aligned_storage_t<sizeof(T), alignof(T)> storage; ///< Storage for an object of type T.
        bool isFree = true; ///< Indicates whether this block is currently free.
    };

    std::vector<MemoryBlock> memoryBlocks_; ///< The pre-allocated memory blocks.
    std::size_t nextFreeIndex_{0}; ///< Index of the next free memory block.
    std::size_t freeBlocksCount_; ///< Number of free blocks in the pool.
    std::size_t lastFreedIndex_; ///< Index of the most recently freed block.
};

} // namespace utils

#endif // MEMORY_POOL_H
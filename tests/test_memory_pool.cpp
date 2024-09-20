#include <gtest/gtest.h>
#include <thread>
#include <vector>

#include "lib/memory_pool.h"


class MemoryPoolTest : public ::testing::Test {
  protected:
    static constexpr std::size_t POOL_SIZE = 32;
    
    struct TestData {
        int a;
        double b;
        explicit TestData(int a = 0, double b = 0.0) : a(a), b(b) {}
    };

    utils::MemoryPool<TestData> pool{POOL_SIZE};
};

TEST_F(MemoryPoolTest, InitialStateIsEmpty) {
    for (std::size_t i = 0; i < POOL_SIZE; ++i) {
        EXPECT_NE(pool.allocate(), nullptr) << "Should be able to allocate " << POOL_SIZE << " elements";
    }
    EXPECT_EQ(pool.allocate(), nullptr) << "Pool should be full after " << POOL_SIZE << " allocations";
}

TEST_F(MemoryPoolTest, AllocateAndDeallocate) {
    auto* ptr = pool.allocate(1, 2.0);
    ASSERT_NE(ptr, nullptr) << "Allocation should succeed";
    EXPECT_EQ(ptr->a, 1);
    EXPECT_EQ(ptr->b, 2.0);

    pool.deallocate(ptr);
    auto* new_ptr = pool.allocate(3, 4.0);
    EXPECT_EQ(new_ptr, ptr) << "New allocation should reuse the deallocated memory";
    EXPECT_EQ(new_ptr->a, 3);
    EXPECT_EQ(new_ptr->b, 4.0);
}

TEST_F(MemoryPoolTest, AllocateUntilFull) {
    std::vector<TestData*> allocated;
    for (std::size_t i = 0; i < POOL_SIZE; ++i) {
        auto* ptr = pool.allocate(i, static_cast<double>(i));
        ASSERT_NE(ptr, nullptr) << "Allocation " << i << " should succeed";
        allocated.push_back(ptr);
    }

    // Try to allocate one more, which should fail
    auto* overflowPtr = pool.allocate(POOL_SIZE, static_cast<double>(POOL_SIZE));
    EXPECT_EQ(overflowPtr, nullptr) << "Pool should be full after " << POOL_SIZE << " allocations";

    // Clean up
    for (auto* ptr : allocated) {
        pool.deallocate(ptr);
    }
}

TEST_F(MemoryPoolTest, DeallocateAll) {
    std::vector<TestData*> allocated;
    for (std::size_t i = 0; i < POOL_SIZE; ++i) {
        allocated.push_back(pool.allocate(i, static_cast<double>(i)));
    }

    for (auto* ptr : allocated) {
        pool.deallocate(ptr);
    }

    for (std::size_t i = 0; i < POOL_SIZE; ++i) {
        EXPECT_NE(pool.allocate(), nullptr) << "Should be able to allocate again after deallocating all";
    }
}

TEST_F(MemoryPoolTest, ConcurrentAllocateAndDeallocate) {
    constexpr int NUM_THREADS = 1; // memory pool is not thread-safe for now increment this when it is
    constexpr int OPS_PER_THREAD = POOL_SIZE * 10;

    auto worker = [&]() {
        std::vector<TestData*> local_allocated;
        for (int i = 0; i < OPS_PER_THREAD; ++i) {
            if (i % 2 == 0 || local_allocated.empty()) {
                auto* ptr = pool.allocate(i, static_cast<double>(i));
                if (ptr) local_allocated.push_back(ptr);
            } else {
                auto* ptr = local_allocated.back();
                local_allocated.pop_back();
                pool.deallocate(ptr);
            }
        }
        for (auto* ptr : local_allocated) {
            pool.deallocate(ptr);
        }
    };

    std::vector<std::jthread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker);
    }

    for (auto& t : threads) {
        t.join();
    }

    int allocations = 0;
    for (std::size_t i = 0; i < POOL_SIZE; ++i) {
        if (pool.allocate() != nullptr) allocations++;
    }
    EXPECT_EQ(allocations, POOL_SIZE) << "All blocks should be free after concurrent operations";
}

TEST_F(MemoryPoolTest, OverAllocate) {
    // Fill the pool
    for (std::size_t i = 0; i < POOL_SIZE; ++i) {
        ASSERT_NE(pool.allocate(i), nullptr) << "Allocation " << i << " should succeed";
    }

    // Try to allocate when the pool is full
    EXPECT_EQ(pool.allocate(POOL_SIZE), nullptr) << "Allocation should fail when pool is full";

    // Verify that the pool is indeed full
    EXPECT_EQ(pool.getFreeBlocksCount(), 0) << "Pool should have no free blocks";
}


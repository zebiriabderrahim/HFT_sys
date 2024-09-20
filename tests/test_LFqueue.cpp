#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>

#include "lib/lock_free_queue.h"

class LFQueueTest : public ::testing::Test {
  protected:
    static constexpr std::size_t QUEUE_SIZE = 100;
    utils::LFQueue<int> queue{QUEUE_SIZE};

    void SetUp() override {
        // Any setup code
    }

    void TearDown() override {
        // Any cleanup code
    }
};

TEST_F(LFQueueTest, InitialStateIsEmpty) {
    SCOPED_TRACE("Checking initial state of the queue");
    EXPECT_EQ(queue.size(), 0) << "Queue should be empty upon initialization";
}

TEST_F(LFQueueTest, PushAndPopSingleElement) {
    SCOPED_TRACE("Testing push and pop of a single element");

    EXPECT_TRUE(queue.push(42)) << "Should be able to push to an empty queue";
    EXPECT_EQ(queue.size(), 1) << "Queue size should be 1 after pushing one element";

    auto result = queue.pop();
    EXPECT_TRUE(result.has_value()) << "Pop should return a value";
    EXPECT_EQ(result.value(), 42) << "Popped value should match pushed value";
    EXPECT_EQ(queue.size(), 0) << "Queue should be empty after popping the only element";
}

TEST_F(LFQueueTest, PushUntilFull) {
    SCOPED_TRACE("Testing pushing elements until the queue is full");

    for (std::size_t i = 0; i < QUEUE_SIZE; ++i) {
        EXPECT_TRUE(queue.push(i)) << "Should be able to push element " << i;
    }
    EXPECT_EQ(queue.size(), QUEUE_SIZE) << "Queue size should equal QUEUE_SIZE after pushing QUEUE_SIZE elements";
    EXPECT_FALSE(queue.push(QUEUE_SIZE)) << "Should fail to push when queue is full";
}

TEST_F(LFQueueTest, PopUntilEmpty) {
    SCOPED_TRACE("Testing popping elements until the queue is empty");

    for (std::size_t i = 0; i < QUEUE_SIZE; ++i) {
        EXPECT_TRUE(queue.push(i));
    }

    for (std::size_t i = 0; i < QUEUE_SIZE; ++i) {
        auto result = queue.pop();
        EXPECT_TRUE(result.has_value()) << "Should be able to pop element " << i;
        EXPECT_EQ(result.value(), i) << "Popped value should match pushed value for element " << i;
    }

    EXPECT_EQ(queue.size(), 0) << "Queue should be empty after popping all elements";
    EXPECT_FALSE(queue.pop().has_value()) << "Pop should return nullopt when queue is empty";
}

TEST_F(LFQueueTest, ConcurrentPushPop) {
    SCOPED_TRACE("Testing concurrent push and pop operations");

    std::atomic<int> sum{0};
    std::atomic<int> count{0};
    constexpr int NUM_OPERATIONS = 10000;

    auto producer = [&]() {
        for (int i = 1; i <= NUM_OPERATIONS; ++i) {
            while (!queue.push(i)) {
                std::this_thread::yield();
            }
        }
    };

    auto consumer = [&]() {
        for (int i = 0; i < NUM_OPERATIONS; ++i) {
            std::optional<int> value;
            while (!(value = queue.pop())) {
                std::this_thread::yield();
            }
            sum += value.value();
            count++;
        }
    };

    std::jthread prod_thread(producer);
    std::jthread cons_thread(consumer);

    prod_thread.join();
    cons_thread.join();

    EXPECT_EQ(count.load(), NUM_OPERATIONS) << "Consumer should have processed all produced items";
    EXPECT_EQ(sum.load(), (NUM_OPERATIONS * (NUM_OPERATIONS + 1)) / 2) << "Sum of consumed values should match expected sum";
}

class LFQueueParamTest : public LFQueueTest, public ::testing::WithParamInterface<int> {};

TEST_P(LFQueueParamTest, PushPopMultipleElements) {
    int num_elements = GetParam();
    SCOPED_TRACE("Testing push and pop of " + std::to_string(num_elements) + " elements");

    for (int i = 0; i < num_elements; ++i) {
        EXPECT_TRUE(queue.push(i)) << "Failed to push element " << i;
    }

    EXPECT_EQ(queue.size(), static_cast<size_t>(num_elements)) << "Queue size should match number of pushed elements";

    for (int i = 0; i < num_elements; ++i) {
        auto result = queue.pop();
        EXPECT_TRUE(result.has_value()) << "Failed to pop element " << i;
        EXPECT_EQ(result.value(), i) << "Popped value should match pushed value for element " << i;
    }

    EXPECT_EQ(queue.size(), 0) << "Queue should be empty after popping all elements";
}

INSTANTIATE_TEST_SUITE_P(
    VaryingElementCounts,
    LFQueueParamTest,
    ::testing::Values(1, 10, 50, 99, 100)
);

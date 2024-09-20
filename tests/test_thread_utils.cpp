#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <format>

#include "lib/thread_utils.h"

using namespace utils;

class ThreadUtilTest : public ::testing::Test {
  protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Platform-independent tests

TEST_F(ThreadUtilTest, CreateAndStartThreadBasic) {
    std::atomic<bool> threadRan{false};

    auto thread = createAndStartThread(-1, "TestThread", [&]() {
        threadRan = true;
    });

    thread->join();

    EXPECT_TRUE(threadRan) << "Thread did not run as expected";
}

TEST_F(ThreadUtilTest, CreateAndStartThreadWithArguments) {
    std::atomic<int> result{0};

    auto thread = createAndStartThread(-1, "TestThread", [](int a, int b, std::atomic<int>& res) {
        res = a + b;
    }, 2, 3, std::ref(result));

    thread->join();

    EXPECT_EQ(result, 5) << "Thread did not process arguments correctly";
}

TEST_F(ThreadUtilTest, MultipleThreads) {
    constexpr int NUM_THREADS = 4;
    std::array<std::atomic<bool>, NUM_THREADS> threadsRan{};

    std::array<threadPtr, NUM_THREADS> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads[i] = createAndStartThread(-1, std::format("TestThread {}", std::to_string(i)), [&threadsRan, i]() {
            threadsRan[i] = true;
        });
    }

    for (auto const& thread : threads) {
        thread->join();
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        EXPECT_TRUE(threadsRan[i]) << "Thread " << i << " did not run";
    }
}

// Linux-specific tests
#ifdef __linux__

TEST_F(ThreadUtilTest, SetThreadCoreAffinity) {
    bool result = setThreadCoreAffinity(0);
    EXPECT_TRUE(result) << "Failed to set thread affinity. This might be due to insufficient permissions or invalid core ID.";

    // Verify the affinity was set
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    int ret = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    ASSERT_EQ(ret, 0) << "Failed to get thread affinity: " << strerror(errno);
    EXPECT_TRUE(CPU_ISSET(0, &cpuset)) << "Thread affinity was not set to core 0 as expected";
}

TEST_F(ThreadUtilTest, CreateAndStartThreadWithAffinity) {
    std::atomic<bool> threadRan{false};
    std::atomic<int> coreId{-1};

    auto thread = createAndStartThread(0, "TestThread", [&]() {
        threadRan = true;
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        int ret = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
        if (ret == 0) {
            for (int i = 0; i < CPU_SETSIZE; ++i) {
                if (CPU_ISSET(i, &cpuset)) {
                    coreId = i;
                    break;
                }
            }
        }
    });

    thread->join();

    EXPECT_TRUE(threadRan) << "Thread did not run";
    EXPECT_EQ(coreId, 0) << "Thread did not run on the expected core";
}

TEST_F(ThreadUtilTest, TerminateOnFailedAffinity) {
    EXPECT_DEATH({
        createAndStartThread(99999, "TestThread", []() {
            // This should never run
        });
    }, "") << "Thread did not terminate as expected when setting an invalid core affinity";
}

#endif // __linux__

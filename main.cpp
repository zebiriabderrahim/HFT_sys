//
// Created by ABDERRAHIM ZEBIRI on 2024-08-06.
//

#include "./utils/memory_pool.h"
#include "./utils/thread_util.h"
#include "utils/lock_free_queue.h"
#include "utils/logger.h"

struct MyStruct {
    int d_[3];
};

using namespace utils;

auto consumeFunction(LFQueue<MyStruct>* lfq) {
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(5s);

    while(lfq->size()) {
        auto d = lfq->pop();

        std::cout << "consumeFunction read elem:" << d->d_[0] << "," << d->d_[1] << "," << d->d_[2] << " lfq-size:" << lfq->size() << std::endl;

        std::this_thread::sleep_for(1s);
    }

    std::cout << "consumeFunction exiting." << std::endl;
}

//int main(int, char **) {
//    LFQueue<MyStruct> lfq(20);

//    auto ct = createAndStartThread(-1, "", consumeFunction, &lfq);
//
//    for(auto i = 0; i < 50; ++i) {
//        const MyStruct d{i, i * 10, i * 100};
//        lfq.push(d);
//
//        std::cout << "main constructed elem:" << d.d_[0] << "," << d.d_[1] << "," << d.d_[2] << " lfq-size:" << lfq.size() << std::endl;
//
//        using namespace std::literals::chrono_literals;
//        std::this_thread::sleep_for(1s);
//    }
//
//    std::cout << "main exiting." << std::endl;

//    return 0;
//}

//
//int main(int, char **) {
//    using namespace utils;
//
//    MemoryPool<float> prim_pool(50);
//    MemoryPool<MyStruct> struct_pool(50);
//
//    for(auto i = 0; i < 50; ++i) {
//        auto p_ret = prim_pool.allocate(2);
//        auto s_ret = struct_pool.allocate(MyStruct{i, i+1, i+2});
//
//        std::cout << "prim elem:" << *p_ret << " allocated at:" << p_ret << std::endl;
//        std::cout << "struct elem:" << s_ret->d_[0] << "," << s_ret->d_[1] << "," << s_ret->d_[2] << " allocated at:" << s_ret << std::endl;
//
//        if(i % 5 == 0) {
//            std::cout << "deallocating prim elem:" << *p_ret << " from:" << p_ret << std::endl;
//            std::cout << "deallocating struct elem:" << s_ret->d_[0] << "," << s_ret->d_[1] << "," << s_ret->d_[2] << " from:" << s_ret << std::endl;
//
//            prim_pool.deallocate(p_ret);
//            struct_pool.deallocate(s_ret);
//        }
//    }
//
//    return 0;
//}

int main() {
    try {
        using enum utils::LogLevel;
        // Create a logger

        // Log different types of messages
        LOG_DEBUG("Simple debug message");
        LOG_INFOF("Formatted info: value = {}", 42);
        // Log some more messages
        for (int i = 0; i < 10; ++i) {
            LOG_INFOF("Iteration {}", i);
        }
        std::string result = std::format("The answer is: {}", 42);

        LOG_INFO(result);

        std::cout << "Logging complete. Waiting for logger to finish..." << std::endl;

        // Wait a bit to allow the logger to process all messages
        std::this_thread::sleep_for(std::chrono::seconds(2));

        std::cout << "Logger test complete. Check test_log.txt for output." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
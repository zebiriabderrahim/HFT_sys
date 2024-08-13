//
// Created by ABDERRAHIM ZEBIRI on 2024-08-06.
//

#include "./utils/memory_pool.h"
#include "./utils/thread_util.h"
#include "utils/Logger.h"
#include "utils/lock_free_queue.h"

struct MyStruct {
    int d_[3];
};

using namespace utils;

auto consumeFunction(LFQueue<MyStruct>* lfq) {
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(5s);

    while(lfq->size()) {
        const auto d = lfq->getNextToRead();
        lfq->updateReadIndex();

        std::cout << "consumeFunction read elem:" << d->d_[0] << "," << d->d_[1] << "," << d->d_[2] << " lfq-size:" << lfq->size() << std::endl;

        std::this_thread::sleep_for(1s);
    }

    std::cout << "consumeFunction exiting." << std::endl;
}

int main(int, char **) {
    LFQueue<MyStruct> lfq(20);

    auto ct = createAndStartThread(-1, "", consumeFunction, &lfq);

    for(auto i = 0; i < 50; ++i) {
        const MyStruct d{i, i * 10, i * 100};
        *(lfq.getNextToWrite()) = d;
        lfq.updateWriteIndex();

        std::cout << "main constructed elem:" << d.d_[0] << "," << d.d_[1] << "," << d.d_[2] << " lfq-size:" << lfq.size() << std::endl;

        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);
    }

    std::cout << "main exiting." << std::endl;

    char c = 'd';
    int i = 3;
    unsigned long ul = 65;
    float f = 3.4;
    double d = 34.56;
    const char* s = "test C-string";
    std::string ss = "test string";

    Logger logger("logging_example.log");

    logger.log(LogLevel::INFO,"Logging a char:% an int:% and an unsigned:%\n", c, i, ul);
    logger.log(LogLevel::INFO,"Logging a float:% and a double:%\n", f, d);
    logger.log(LogLevel::INFO,"Logging a C-string:'%'\n", s);
    logger.log(LogLevel::INFO,"Logging a string:'%'\n", ss);

    return 0;
}

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
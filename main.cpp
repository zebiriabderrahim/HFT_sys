//
// Created by ABDERRAHIM ZEBIRI on 2024-08-06.
//


#include "./utils/thread_util.h"
#include "./utils/memory_pool.h"

auto dummyFunction(int a, int b, bool sleep) {
    std::cout << "dummyFunction(" << a << "," << b << ")" << std::endl;
    std::cout << "dummyFunction output=" << a + b << std::endl;

    if(sleep) {
        std::cout << "dummyFunction sleeping..." << std::endl;

        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(5s);
    }

    std::cout << "dummyFunction done." << std::endl;
}

//int main(int, char **) {
//    using namespace utils;
//
//    auto t1 = createAndStartThread(-1, "dummyFunction1", dummyFunction, 12, 21, false);
//    auto t2 = createAndStartThread(-1, "dummyFunction2", dummyFunction, 15, 51, true);
//
//    std::cout << "main exiting." << std::endl;
//
//    return 0;
//}
struct MyStruct {
    int d_[3];
};

int main(int, char **) {
    using namespace utils;

    MemoryPool<double> prim_pool(50);
    MemoryPool<MyStruct> struct_pool(50);

    for(auto i = 0; i < 50; ++i) {
        auto p_ret = prim_pool.allocate(1);
        auto s_ret = struct_pool.allocate(MyStruct{i, i+1, i+2});

        std::cout << "prim elem:" << *p_ret << " allocated at:" << p_ret << std::endl;
        std::cout << "struct elem:" << s_ret->d_[0] << "," << s_ret->d_[1] << "," << s_ret->d_[2] << " allocated at:" << s_ret << std::endl;

        if(i % 5 == 0) {
            std::cout << "deallocating prim elem:" << *p_ret << " from:" << p_ret << std::endl;
            std::cout << "deallocating struct elem:" << s_ret->d_[0] << "," << s_ret->d_[1] << "," << s_ret->d_[2] << " from:" << s_ret << std::endl;

            prim_pool.deallocate(p_ret);
            struct_pool.deallocate(s_ret);
        }
    }

    return 0;
}
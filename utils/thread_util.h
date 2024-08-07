//
// Created by ABDERRAHIM ZEBIRI on 2024-08-06.
//

#ifndef LOW_LATENCY_TRADING_APP_THREAD_UTIL_H
#define LOW_LATENCY_TRADING_APP_THREAD_UTIL_H

#include <iostream>
#include <thread>
#include <atomic>
#include <unistd.h>
#include <sys/syscall.h>


#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#elif defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>
#include <mach/thread_policy.h>
#else
#include <pthread.h>
#include <sched.h>
#endif


namespace utils {

inline auto set_thread_core_affinity(int core_id) -> bool{

#if defined(_WIN32) || defined(_WIN64)
    // Windows implementation
    DWORD_PTR mask = 1 << core_id;
    HANDLE thread = GetCurrentThread();
    return SetThreadAffinityMask(thread, mask) != 0;
#elif defined(__APPLE__) && defined(__MACH__)
    // macOS implementation
    thread_affinity_policy_data_t policy = { core_id };
    thread_port_t mach_thread = pthread_mach_thread_np(pthread_self());
    return thread_policy_set(mach_thread,
                             THREAD_AFFINITY_POLICY,
                             (thread_policy_t)&policy,
                             1) == KERN_SUCCESS;
#else
    // Linux implementation
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
#endif
}

inline auto get_thread_core_affinity() noexcept -> std::jthread{

}


}
#endif //LOW_LATENCY_TRADING_APP_THREAD_UTIL_H

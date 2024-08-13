//
// Created by ABDERRAHIM ZEBIRI on 2024-08-06.
//

#ifndef LOW_LATENCY_TRADING_APP_THREAD_UTIL_H
#define LOW_LATENCY_TRADING_APP_THREAD_UTIL_H

#include <chrono>
#include <concepts>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>

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

inline auto setThreadCoreAffinity(int core_id) -> bool {

#if defined(_WIN32) || defined(_WIN64)
    // Windows implementation
    DWORD_PTR mask = 1 << core_id;
    HANDLE thread = GetCurrentThread();
    return SetThreadAffinityMask(thread, mask) != 0;
#elif defined(__APPLE__) && defined(__MACH__)
    // macOS implementation
    thread_affinity_policy_data_t policy = {core_id};
    thread_port_t mach_thread = pthread_mach_thread_np(pthread_self());

    kern_return_t result = thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, (thread_policy_t) &policy, THREAD_AFFINITY_POLICY_COUNT);

    return result == KERN_SUCCESS;
#else
    // Linux implementation
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
#endif
}
using threadPtr = std::unique_ptr<std::jthread>;
template <typename F, typename... Args>
concept Invocable = std::invocable<F, Args...>;

template <typename F, typename... Args>
    requires Invocable<F, Args...>
[[nodiscard]] inline auto createAndStartThread(int core_id, std::string_view name, F &&func, Args &&...args) noexcept -> threadPtr {
    auto f = [core_id, name, function = std::forward<F>(func), ... arguments = std::forward<Args>(args)]() mutable {
        if (core_id >= 0 && !setThreadCoreAffinity(core_id)) {
            std::cerr << "Failed to set core affinity for " << name << " " << std::this_thread::get_id() << " to " << core_id << '\n';
            std::terminate();
        }

        std::cout << "Set core affinity for " << name << " " << std::this_thread::get_id() << " to " << core_id << '\n';

        std::invoke(function, arguments...);
    };
    return std::make_unique<std::jthread>(f);
}
} // namespace utils
#endif // LOW_LATENCY_TRADING_APP_THREAD_UTIL_H

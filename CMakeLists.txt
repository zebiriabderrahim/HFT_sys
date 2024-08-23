cmake_minimum_required(VERSION 3.10)
project(low_latency_trading_app)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Remove incompatible compiler flags

add_executable(low_latency_trading_app  main.cpp
                                        utils/thread_util.h
                                        utils/memory_pool.h
                                        utils/debug_assertion.h
                                        utils/lock_free_queue.h
                                        utils/time_utils.h
        utils/logger.cpp
        utils/logger.h
        utils/socket_utils.cpp
        utils/socket_utils.h
)
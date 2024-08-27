cmake_minimum_required(VERSION 3.10)
project(low_latency_trading_app)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#add_subdirectory(benchmark)

add_executable(low_latency_trading_app  main.cpp
                                        utils/thread_util.h
                                        utils/memory_pool.h
                                        utils/debug_assertion.h
                                        utils/lock_free_queue.h
                                        utils/time_utils.h
                                        utils/logger.cpp
                                        utils/logger.h
                                        utils/socket_utils.h
        utils/tcp_socket.cpp
        utils/tcp_socket.h
        utils/tcp_server.cpp
        utils/tcp_server.h
)

#target_link_libraries(low_latency_trading_app PRIVATE benchmark::benchmark pthread stdc++)
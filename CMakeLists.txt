cmake_minimum_required(VERSION 3.27)

project(HFT_sys, VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-std=c++2a -Wall -Wextra -Werror -Wpedantic")
set(CMAKE_VERBOSE_MAKEFILE on)

add_subdirectory(src/lib)
add_subdirectory(src/core)

list(APPEND LIBS lib)
list(APPEND LIBS core)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/core)

file(GLOB HFT_sys_SOURCES "src/core/*.cpp")
add_executable(
        HFT_sys
        ${HFT_sys_SOURCES}
)
target_link_libraries(HFT_sys PUBLIC ${LIBS})

# test target with Gtest
file(GLOB TEST_SOURCES "tests/*.cpp")
# FetchContent to load and build googletest
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG main
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

# declare and link tests target
enable_testing()
add_executable(
        tests
        ${TEST_SOURCES}
)
target_link_libraries(tests PRIVATE gtest gtest_main ${LIBS})

include(GoogleTest)
gtest_discover_tests(tests)
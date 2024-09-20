set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-std=c++2a -Wall -Wextra -Werror -Wpedantic")
set(CMAKE_VERBOSE_MAKEFILE on)

file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

include_directories(${PROJECT_SOURCE_DIR}/src/lib)

add_library(lib STATIC ${SOURCES} ${HEADERS})
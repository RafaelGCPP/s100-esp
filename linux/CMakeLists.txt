cmake_minimum_required(VERSION 3.21)

project(s100_linux LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
  chips
  GIT_REPOSITORY https://github.com/floooh/chips.git
  GIT_TAG master
)

FetchContent_MakeAvailable(chips)

add_library(chips_impl STATIC chips_impl.c)
target_include_directories(chips_impl PUBLIC ${chips_SOURCE_DIR})

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../components/cpu ${CMAKE_BINARY_DIR}/components/cpu)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../components/memory ${CMAKE_BINARY_DIR}/components/memory)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../components/disk ${CMAKE_BINARY_DIR}/components/disk)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../components/uart ${CMAKE_BINARY_DIR}/components/uart)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../components/machine ${CMAKE_BINARY_DIR}/components/machine)

add_executable(s100_linux main.cpp)
target_include_directories(s100_linux PRIVATE ${chips_SOURCE_DIR})
target_link_libraries(s100_linux PRIVATE chips_impl)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../experiments/z80_hello ${CMAKE_BINARY_DIR}/experiments/z80_hello)

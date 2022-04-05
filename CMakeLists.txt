cmake_minimum_required(VERSION 3.15)
project(pitchtool VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_TESTS "Enable building Tests" ON)

add_subdirectory(backend)
if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()

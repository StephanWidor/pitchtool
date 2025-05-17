cmake_minimum_required(VERSION 3.20)
project(pitchtool VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_JUCE_PLUGIN "Build JUCE Plugin" ON)
option(ENABLE_TESTS "Enable building Tests" OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif(NOT CMAKE_BUILD_TYPE)

add_subdirectory(backend)
if(BUILD_JUCE_PLUGIN)
    add_subdirectory(juce)
endif()
if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()

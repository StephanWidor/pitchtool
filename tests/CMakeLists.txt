cmake_minimum_required(VERSION 3.20)
project(pitchtool-tests LANGUAGES CXX)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()
include(GoogleTest)

add_executable(${PROJECT_NAME}
    sw/dft/spectrum.cpp
    sw/dft/transform.cpp
    sw/notes.cpp
    sw/phases.cpp
    sw/sinewavedetection.cpp
    sw/pitchprocessor.cpp
    )

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

target_link_libraries(${PROJECT_NAME}
    pitchtool-backend
    gtest
    gtest_main
    gmock
    gmock_main
    $<$<BOOL:Python3_FOUND>:Python3::Python>
    $<$<BOOL:Python3_FOUND>:Python3::Module>
    )

gtest_discover_tests(${PROJECT_NAME})

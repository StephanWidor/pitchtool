cmake_minimum_required(VERSION 3.20)
project(pitchtool-backend LANGUAGES CXX)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(swAudioLib)

add_library(${PROJECT_NAME} INTERFACE
    sw/pitchtool/processor.hpp
    sw/pitchtool/types.hpp
    )

target_link_libraries(${PROJECT_NAME}
    INTERFACE
    swAudioLib
    )

target_include_directories(${PROJECT_NAME} INTERFACE .)

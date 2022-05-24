cmake_minimum_required(VERSION 3.16)
project(pitchtool-backend LANGUAGES CXX)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} INTERFACE
    sw/basics/chrono.hpp
    sw/basics/math.hpp
    sw/basics/variant.hpp
    sw/basics/ranges.hpp
    sw/containers/spinlockedbuffer.hpp
    sw/containers/utils.hpp
    sw/dft/spectrum.hpp
    sw/dft/transform.hpp
    sw/dft/utils.hpp
    sw/frequencyfilter.hpp
    sw/notes.hpp
    sw/phases.hpp
    sw/pitchtool/processor.hpp
    sw/pitchtool/types.hpp
    sw/processingbuffer.hpp
    sw/signals.hpp
    sw/spectrum.hpp
    sw/tuningnoteenvelope.hpp
    )

find_package(OpenMP)
target_link_libraries(${PROJECT_NAME}
    INTERFACE
    $<$<BOOL:OpenMP_CXX_FOUND>:OpenMP::OpenMP_CXX>
    )

target_include_directories(${PROJECT_NAME} INTERFACE .)

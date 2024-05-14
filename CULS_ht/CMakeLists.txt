cmake_minimum_required(VERSION 3.8.0)

set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_C_COMPILER "/usr/bin/gcc")

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

set(GPULS_CUDA_FLAGS -arch=compute_60 -code=compute_60,sm_60,sm_86)
set(GPULS_CUDA_FLAGS ${GPULS_CUDA_FLAGS} -std=c++17 -O3 --extended-lambda)
set(CMAKE_CXX_STANDARD 17)

file(GLOB GPULS_GPU_SOURCES 
    "src/*.cu"
    "src/strash/*.cu")

file(GLOB GPULS_CPU_SOURCES 
    "src/*.hpp"
    "src/*.cpp")

set(MAIN_SRC "src/main.cu")
list(FILTER GPULS_GPU_SOURCES EXCLUDE REGEX ".*/src/main\.cu")

# message(${GPULS_GPU_SOURCES} ${GPULS_CPU_SOURCES} ${MAIN_SRC})
project(ghash C CXX CUDA)

add_library(glib SHARED ${GPULS_GPU_SOURCES})
target_include_directories(glib PUBLIC "src")
target_include_directories(glib PUBLIC "src/hash_table")
target_include_directories(glib PUBLIC "src/strash")
target_include_directories(glib PUBLIC "src/include")

add_executable(ghash ${MAIN_SRC})

target_include_directories(ghash PUBLIC "src")
target_include_directories(ghash PUBLIC "src/strash")

target_link_libraries(ghash PRIVATE glib)

target_compile_options(ghash PUBLIC $<$<COMPILE_LANGUAGE:CUDA>:${GPULS_CUDA_FLAGS}>)

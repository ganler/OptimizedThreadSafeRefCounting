cmake_minimum_required(VERSION 3.14)
project(OptimizedThreadSafeRefCounting)

set(CMAKE_CXX_STANDARD 17)
include_directories(.)

add_executable(OptimizedThreadSafeRefCounting main.cpp ganler_ref_cnter.hpp)
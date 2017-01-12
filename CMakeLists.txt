cmake_minimum_required(VERSION 3.6)
project(FileSystem)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp FileSystem.cpp FileSystem.h)
add_executable(FileSystem ${SOURCE_FILES})
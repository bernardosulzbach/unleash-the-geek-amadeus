cmake_minimum_required(VERSION 2.8)

project(unleash-the-geek-amadeus)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_definitions(-Wall)
add_definitions(-Wextra)
add_definitions(-Wno-unused-variable)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions(-DDEBUGGING)
endif()

add_executable(unleash-the-geek-amadeus main.cpp)

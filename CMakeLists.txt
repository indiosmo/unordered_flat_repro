cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(unordered_flat_repro LANGUAGES CXX)

find_package(Boost 1.85 REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(repro main.cpp)
  
target_link_libraries(repro
  PRIVATE
    Boost::boost
)

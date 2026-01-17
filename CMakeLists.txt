cmake_minimum_required(VERSION 3.16)
project(HIGH_PERFORMANCE_LOB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(LOB
    src/Book.cpp
    src/Limit.cpp
    src/Order.cpp
)

target_include_directories(lob
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

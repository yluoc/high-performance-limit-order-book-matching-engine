cmake_minimum_required(VERSION 3.16)
project(HIGH_PERFORMANCE_LOB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Main executable
add_executable(LOB
    src/Book.cpp
    src/Limit.cpp
    src/Order.cpp
)

target_include_directories(LOB PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Google Test - use system installation
find_package(GTest REQUIRED)

# Test executable
add_executable(LOBTest
    test/LOBTest.cpp
    src/Book.cpp
    src/Limit.cpp
    src/Order.cpp
)

target_include_directories(LOBTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(LOBTest
    GTest::gtest
    GTest::gtest_main
)

enable_testing()
add_test(NAME LOBTest COMMAND LOBTest)

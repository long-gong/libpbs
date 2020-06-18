cmake_minimum_required(VERSION 3.0)

project(libpbs)

set(CMAKE_CXX_STANDARD 17)

include_directories(src 3rd/include)
enable_testing()
add_executable(test_pbs_params test/test_pbs_params.cpp)
target_link_libraries(test_pbs_params gtest gtest_main pthread)

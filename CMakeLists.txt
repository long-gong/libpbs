cmake_minimum_required(VERSION 3.0)

project(libpbs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O3 -fPIC -Wall")
set(PROJECT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(THIRD_PARTY 3rd)

if (NOT CMAKE_BUILD_TYPE)
    message("No build type is provided, using release")
    set(CMAKE_BUILD_TYPE "Release")
endif ()

if (CMAKE_BUILD_TYPE AND
        NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release)$")
    message(FATAL_ERROR "Invalid value for CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("Debug Mode")
    set(CMAKE_CXX_FLAGS "-g -O0 -fPIC -Wall")
endif ()

include_directories(src 3rd/include)

find_package(Boost REQUIRED COMPONENTS serialization filesystem)
find_package(Eigen3 REQUIRED)
find_package(GTest REQUIRED)
find_package(fmt REQUIRED)

add_subdirectory(src)


##### For DDigest & Graphene ###
set(ddigest_objs "")
add_library(murmur_hash OBJECT 3rd/include/iblt/murmurhash3.cpp)
add_library(utilstrencodings OBJECT 3rd/include/iblt/utilstrencodings.cpp)
add_library(iblt OBJECT 3rd/include/iblt/iblt.cpp)
list(APPEND ddigest_objs murmur_hash utilstrencodings iblt)

message(STATUS "DDigest objects: ${ddigest_objs} ...")

####### TESTS ########
enable_testing()

add_executable(test_eigen_boost_serialization
        test/test_eigen_boost_serialization.cpp)
target_link_libraries(test_eigen_boost_serialization GTest::GTest GTest::Main
        Boost::serialization Boost::filesystem Eigen3::Eigen)

add_executable(test_cache_helper test/test_cache_helper.cpp)
target_link_libraries(
        test_cache_helper
        GTest::GTest
        GTest::Main
        Boost::serialization
        Boost::filesystem
        Eigen3::Eigen
        fmt::fmt)

add_executable(test_pbs_params test/test_pbs_params.cpp)
target_link_libraries(
        test_pbs_params
        GTest::GTest
        GTest::Main
        Boost::serialization
        Boost::filesystem
        Eigen3::Eigen
        fmt::fmt)

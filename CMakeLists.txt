cmake_minimum_required(VERSION 3.0)

project(libpbs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O3 -fPIC -Wall")
set(PROJECT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(THIRD_PARTY 3rd)

include_directories(src 3rd/include)

find_package(Boost REQUIRED COMPONENTS serialization filesystem)
find_package(Eigen3 REQUIRED)
find_package(GTest REQUIRED)
find_package(fmt REQUIRED)

add_subdirectory(src)


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

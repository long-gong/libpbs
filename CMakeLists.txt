cmake_minimum_required(VERSION 3.0)

project(libpbs)

set(CMAKE_CXX_STANDARD 17)

include_directories(src 3rd/include)
enable_testing()

find_package(Boost REQUIRED
             COMPONENTS serialization filesystem)
find_package(Eigen3 REQUIRED)
find_package(GTest REQUIRED)
find_package(fmt REQUIRED)


add_executable(test_eigen_boost_serialization test/test_eigen_boost_serialization.cpp)
target_link_libraries(test_eigen_boost_serialization GTest::GTest GTest::Main Boost::serialization Boost::filesystem Eigen3::Eigen)

add_executable(test_cache_helper test/test_cache_helper.cpp)
target_link_libraries(test_cache_helper GTest::GTest GTest::Main Boost::serialization Boost::filesystem Eigen3::Eigen fmt::fmt)


add_executable(test_pbs_params test/test_pbs_params.cpp)
target_link_libraries(test_pbs_params GTest::GTest GTest::Main Boost::serialization Boost::filesystem Eigen3::Eigen fmt::fmt)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

#find_package(PkgConfig REQUIRED)
#pkg_search_module(GRPCPP REQUIRED grpc++>=1.22.0)

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf protobuf::libprotoc protobuf::libprotobuf-lite)

# Proto file
get_filename_component(proto_file "../protos/reconciliation.proto" ABSOLUTE)
get_filename_component(proto_path "${proto_file}" PATH)

# Generated sources
set(proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/reconciliation.pb.cc")
set(proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/reconciliation.pb.h")
set(grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/reconciliation.grpc.pb.cc")
set(grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/reconciliation.grpc.pb.h")

find_program(_PROTOBUF_PROTOC protoc)
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
add_custom_command(
        OUTPUT "${proto_srcs}" "${proto_hdrs}" "${grpc_srcs}" "${grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${proto_file}"
        DEPENDS "${proto_file}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_library(proto_obj OBJECT IMPORTED proto_srcs)
add_library(grpc_obj OBJECT IMPORTED grpc_srcs)


SET(CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath -Wl,${THIRD_PARTY}/lib")

add_executable(server "reconciliation_server.cpp"
        ${proto_srcs}
        ${grpc_srcs}
        ${ddigest_objs})
target_include_directories(server
        PRIVATE
        ${GRPCPP_INCLUDE_DIRS})
target_link_libraries(server
        gRPC::grpc++ gRPC::grpc++_reflection
        ${_PROTOBUF_LIBPROTOBUF}
        xxhash
        minisketch
        GTest::GTest
        GTest::Main
        Boost::serialization
        Boost::filesystem
        Eigen3::Eigen
        fmt::fmt)

add_executable(server995 "reconciliation_server.cpp"
        ${proto_srcs}
        ${grpc_srcs}
        ${ddigest_objs})
target_compile_definitions(server995 PRIVATE -DTARGET_SUCCESS_PROB)
target_include_directories(server995
        PRIVATE
        ${GRPCPP_INCLUDE_DIRS})
target_link_libraries(server995
        gRPC::grpc++ gRPC::grpc++_reflection
        ${_PROTOBUF_LIBPROTOBUF}
        xxhash
        minisketch
        GTest::GTest
        GTest::Main
        Boost::serialization
        Boost::filesystem
        Eigen3::Eigen
        fmt::fmt)

add_executable(client "reconciliation_client.cpp"
        ${proto_srcs}
        ${grpc_srcs}
        ${ddigest_objs})
target_include_directories(client
        PRIVATE
        ${GRPCPP_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(client
        gRPC::grpc++ gRPC::grpc++_reflection
        ${_PROTOBUF_LIBPROTOBUF}
        xxhash
        minisketch
        GTest::GTest
        GTest::Main
        Boost::serialization
        Boost::filesystem
        Eigen3::Eigen
        fmt::fmt)

add_executable(client995 "reconciliation_client.cpp"
        ${proto_srcs}
        ${grpc_srcs}
        ${ddigest_objs})
target_compile_definitions(client995 PRIVATE -DTARGET_SUCCESS_PROB)
target_include_directories(client995
        PRIVATE
        ${GRPCPP_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(client995
        gRPC::grpc++ gRPC::grpc++_reflection
        ${_PROTOBUF_LIBPROTOBUF}
        xxhash
        minisketch
        GTest::GTest
        GTest::Main
        Boost::serialization
        Boost::filesystem
        Eigen3::Eigen
        fmt::fmt)
## TESTS ##
enable_testing()
add_executable(test_pbs_messages "../test/test_pbs_messages.cpp")
target_include_directories(test_pbs_messages PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_pbs_messages
        minisketch
        GTest::GTest
        GTest::Main
        fmt::fmt)
add_test(NAME vtest
        COMMAND valgrind --leak-check=yes --error-exitcode=1 $<TARGET_FILE:test_pbs_messages>)

add_executable(test_pbs "../test/test_pbs.cpp")
#if (CMAKE_BUILD_TYPE STREQUAL "Debug")
#    target_compile_definitions(test_pbs PRIVATE -DDEBUG_PBS)
#endif ()
target_include_directories(test_pbs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_pbs
        minisketch
        GTest::GTest
        GTest::Main
        Boost::serialization
        Boost::filesystem
        Eigen3::Eigen
        fmt::fmt)

# avoid to change source code
configure_file(../3rd/include/iblt/param.export.0.995833.2018-07-17.csv ${CMAKE_CURRENT_BINARY_DIR}/param.export.0.995833333333333.2018-07-12.csv
        COPYONLY)
message("DDigest: ${ddigest_objs}")
add_executable(test_reconciliation_services "../test/test_reconciliation_services.cpp"
        ${proto_srcs}
        ${grpc_srcs}
        ${ddigest_objs})
target_include_directories(test_reconciliation_services
        PRIVATE
        ${GRPCPP_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_reconciliation_services
        gRPC::grpc++ gRPC::grpc++_reflection
        ${_PROTOBUF_LIBPROTOBUF}
        xxhash
        minisketch
        GTest::GTest
        GTest::Main
        Boost::serialization
        Boost::filesystem
        Eigen3::Eigen
        fmt::fmt)


add_executable(test_bench_utils ../test/test_bench_utils.cpp)
target_link_libraries(test_bench_utils
        GTest::GTest
        GTest::Main
        fmt::fmt)


add_executable(test_cli11 ../test/test_cli11.cpp)
target_link_libraries(test_cli11 fmt::fmt)
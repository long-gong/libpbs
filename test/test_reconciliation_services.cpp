#include <gtest/gtest.h>

#include <thread>
#include <future>

#include "reconciliation_client.h"
#include "reconciliation_server.h"

// Somewhere in the global scope :/
std::promise<void> exit_estimation_service;
std::promise<void> exit_pinsketch_service;
std::promise<void> exit_push_pull_service;
std::promise<void> exit_pbs_service;

void stop_estimation_service() {
  exit_estimation_service.set_value();
}

void stop_pinsketch_service() {
  exit_pinsketch_service.set_value();
}

void stop_pbs_service() {
  exit_pbs_service.set_value();
}

void stop_push_pull_service() {
  exit_push_pull_service.set_value();
}

void run_server_for_testing_estimation_service() {
  std::string server_address("0.0.0.0:50051");
  EstimationServiceImpl service;
  std::vector<int> empty_set;
  service.LocalSketchFor(empty_set.begin(), empty_set.end());

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server (testing estimation service) listening on "
            << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  auto serveFn = [&]() {
    server->Wait();
  };

  std::thread serving_thread(serveFn);

  auto f = exit_estimation_service.get_future();
  f.wait();
  server->Shutdown();
  serving_thread.join();
}

void run_server_for_testing_pinsketch_service() {
  std::string server_address("0.0.0.0:50051");
  EstimationServiceImpl service;

  auto server_data_ptr = std::make_shared<tsl::ordered_map<Key, Value>>(tsl::ordered_map<Key, Value>{
      {4, "4444"}, {6, "666666"}, {3, "333"}, {5, "55555"}
  });

  service.set_key_value_pairs(server_data_ptr);
  service.set_estimated_diff(4);

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server (testing pinsketch service) listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  auto serveFn = [&]() {
    server->Wait();
  };

  std::thread serving_thread(serveFn);
  auto f = exit_pinsketch_service.get_future();
  f.wait();
  server->Shutdown();
  serving_thread.join();
}

void run_server_for_testing_pbs_service() {
  std::string server_address("0.0.0.0:50051");
  EstimationServiceImpl service;

  auto server_data_ptr = std::make_shared<tsl::ordered_map<Key, Value>>(tsl::ordered_map<Key, Value>{
      {4, "4444"}, {6, "666666"}, {3, "333"}, {5, "55555"}
  });

  service.set_key_value_pairs(server_data_ptr);
  service.set_estimated_diff(4);

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server (testing pbs service) listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  auto serveFn = [&]() {
    server->Wait();
  };

  std::thread serving_thread(serveFn);
  auto f = exit_pbs_service.get_future();
  f.wait();
  server->Shutdown();
  serving_thread.join();
}

void run_server_for_testing_push_pull_service() {
  std::string server_address("0.0.0.0:50051");
  EstimationServiceImpl service;

  auto server_data_ptr = std::make_shared<tsl::ordered_map<Key, Value>>(tsl::ordered_map<Key, Value>{
      {4, "4444"}, {6, "666666"}, {3, "333"}, {5, "55555"}
  });

  service.set_key_value_pairs(server_data_ptr);
  service.set_estimated_diff(4);

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server (testing push and pull services) listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  auto serveFn = [&]() {
    server->Wait();
  };

  std::thread serving_thread(serveFn);
  auto f = exit_push_pull_service.get_future();
  f.wait();
  server->Shutdown();
  serving_thread.join();
}

TEST(ReconciliationServicesTest, EstimationService) {
  std::thread th_run_server(run_server_for_testing_estimation_service);
  {
    // start a client
    std::string target_str = "localhost:50051";
    ReconciliationClient client(
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    for (size_t n = 8; n < 1024; n *= 2) {
      auto testset = GenerateRandomSet32(n);
      ASSERT_EQ(testset.size(), n);
      auto reply = client.Estimation(testset.begin(), testset.end());
      std::cout << "Estimate :  " << reply << " (" << n << ")" << std::endl;
      std::this_thread::sleep_for(1s);
    }
  }

  stop_estimation_service();
  th_run_server.join();
}

TEST(ReconciliationServicesTest, PinSketchService) {
  std::thread th_run_server(run_server_for_testing_pinsketch_service);
  std::this_thread::sleep_for(1s);// make sure server is ready when client calls
  {
    // start a client
    std::string target_str = "localhost:50051";
    ReconciliationClient client(
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    tsl::ordered_map<Key, Value> client_data{
        {1, "1"}, {2, "22"}, {3, "333"}, {5, "55555"}
    };
    tsl::ordered_map<Key, Value> expected{
        {1, "1"}, {2, "22"}, {3, "333"}, {5, "55555"}, {4, "4444"}, {6, "666666"}
    };
    EXPECT_TRUE(client.Reconciliation_PinSketch(client_data, 4));
    EXPECT_EQ(expected, client_data);
  }

  stop_pinsketch_service();
  th_run_server.join();
}

TEST(ReconciliationServicesTest, ParityBitmapSketchService) {
  std::thread th_run_server(run_server_for_testing_pbs_service);
  std::this_thread::sleep_for(1s);// make sure server is ready when client calls
  {
    // start a client
    std::string target_str = "localhost:50051";
    ReconciliationClient client(
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    tsl::ordered_map<Key, Value> client_data{
        {1, "1"}, {2, "22"}, {3, "333"}, {5, "55555"}
    };
    tsl::ordered_map<Key, Value> expected{
        {1, "1"}, {2, "22"}, {3, "333"}, {5, "55555"}, {4, "4444"}, {6, "666666"}
    };
    EXPECT_TRUE(client.Reconciliation_ParityBitmapSketch(client_data, 4));
    EXPECT_EQ(expected, client_data);
  }

  stop_pbs_service();
  th_run_server.join();
}

TEST(ReconciliationServicesTest, PushPullService) {
  std::thread th_run_server(run_server_for_testing_push_pull_service);
  std::this_thread::sleep_for(1s);// make sure server is ready when client calls
  std::string target_str = "localhost:50051";
  ReconciliationClient client(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  {
    // test pull
    tsl::ordered_set<Key> pull_keys{
        3, 4
    };
    tsl::ordered_map<Key, Value> expected{
        {3, "333"}, {4, "4444"}
    };
    tsl::ordered_map<Key, Value> pulled;
    EXPECT_TRUE(client.Pull(pull_keys.cbegin(), pull_keys.cend(), pulled)) ;
    EXPECT_EQ(expected, pulled);
  }

  {
    // test push
    tsl::ordered_set<Key> push_keys{
        1, 2
    };
    tsl::ordered_map<Key, Value> client_data{
        {1, "1"}, {2, "22"}, {3, "333"}, {5, "55555"}
    };
    tsl::ordered_map<Key, Value> expected{
        {1, "1"}, {2, "22"}
    };
    EXPECT_TRUE(client.Push(push_keys.cbegin(), push_keys.cend(), client_data));
    tsl::ordered_map<Key, Value> pulled;
    EXPECT_TRUE (client.Pull(push_keys.cbegin(), push_keys.cend(), pulled));
    EXPECT_EQ(expected, pulled);
  }

  {
    // test push and pull
    tsl::ordered_set<Key> push_keys{
        7
    };
    tsl::ordered_set<Key> pull_keys{
        3, 5
    };
    tsl::ordered_map<Key, Value> client_data{
        {7, "7777777"}
    };
    tsl::ordered_map<Key, Value> expected{
        {7, "7777777"}, {3, "333"}, {5, "55555"}
    };
    EXPECT_TRUE(client.PushAndPull(push_keys.cbegin(),
                               push_keys.cend(),
                               pull_keys.cbegin(),
                               pull_keys.cend(),
                               client_data));
    EXPECT_EQ(expected, client_data);
  }

  stop_push_pull_service();
  th_run_server.join();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
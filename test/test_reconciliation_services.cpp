#include <gtest/gtest.h>

#include <future>
#include <thread>

#include "reconciliation_client.h"
#include "reconciliation_server.h"

// Somewhere in the global scope :/
std::promise<void> exit_estimation_service;
std::promise<void> exit_pinsketch_service;
std::promise<void> exit_push_pull_service;
std::promise<void> exit_pbs_service;
std::promise<void> exit_ddigest_service;
std::promise<void> exit_graphene_service;

void stop_estimation_service() { exit_estimation_service.set_value(); }

void stop_pinsketch_service() { exit_pinsketch_service.set_value(); }
void reset_pinsketch_service() {
  exit_pinsketch_service = std::promise<void>();
}
void stop_pbs_service() { exit_pbs_service.set_value(); }
void reset_pbs_service() {
  exit_pbs_service = std::promise<void>();
}
void stop_ddigest_service() { exit_ddigest_service.set_value();}
void reset_ddigest_service() {
  exit_ddigest_service = std::promise<void>();
}
void stop_graphene_service() { exit_graphene_service.set_value(); }
void reset_graphene_service() {
  exit_graphene_service = std::promise<void>();
}
void stop_push_pull_service() { exit_push_pull_service.set_value(); }

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
  auto serveFn = [&]() { server->Wait(); };

  std::thread serving_thread(serveFn);

  auto f = exit_estimation_service.get_future();
  f.wait();
  server->Shutdown();
  serving_thread.join();
}

void run_server_for_testing_pinsketch_service() {
  std::string server_address("0.0.0.0:50051");
  EstimationServiceImpl service;

  auto server_data_ptr = std::make_shared<tsl::ordered_map<Key, Value>>(
      tsl::ordered_map<Key, Value>{
          {4, "4444"}, {6, "666666"}, {3, "333"}, {5, "55555"}});

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
  std::cout << "Server (testing pinsketch service) listening on "
            << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  auto serveFn = [&]() { server->Wait(); };

  std::thread serving_thread(serveFn);
  auto f = exit_pinsketch_service.get_future();
  f.wait();
  server->Shutdown();
  serving_thread.join();
}

void run_server_for_testing_ddigest_service() {
  std::string server_address("0.0.0.0:50051");
  EstimationServiceImpl service;

  auto server_data_ptr = std::make_shared<tsl::ordered_map<Key, Value>>(
      tsl::ordered_map<Key, Value>{
          {4, "4444"}, {6, "666666"}, {3, "333"}, {5, "55555"}});

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
  std::cout << "Server (testing ddigest service) listening on "
            << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  auto serveFn = [&]() { server->Wait(); };

  std::thread serving_thread(serveFn);
  auto f = exit_ddigest_service.get_future();
  f.wait();
  server->Shutdown();
  serving_thread.join();
}

void run_server_for_testing_graphene_service() {
  std::string server_address("0.0.0.0:50051");
  EstimationServiceImpl service;

  auto server_data_ptr = std::make_shared<tsl::ordered_map<Key, Value>>(
      tsl::ordered_map<Key, Value>{
           {4, "4444"}, {6, "666666"}, {3, "333"}, {5, "55555"}});

  service.set_key_value_pairs(server_data_ptr);

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
  std::cout << "Server (testing graphene service) listening on "
            << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  auto serveFn = [&]() { server->Wait(); };

  std::thread serving_thread(serveFn);
  auto f = exit_graphene_service.get_future();
  f.wait();
  server->Shutdown();
  serving_thread.join();
}

void run_server_for_testing_pbs_service() {
  std::string server_address("0.0.0.0:50051");
  EstimationServiceImpl service;

  auto server_data_ptr = std::make_shared<tsl::ordered_map<Key, Value>>(
      tsl::ordered_map<Key, Value>{
          {4, "4444"}, {6, "666666"}, {3, "333"}, {5, "55555"}});

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
  std::cout << "Server (testing pbs service) listening on " << server_address
            << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  auto serveFn = [&]() { server->Wait(); };

  std::thread serving_thread(serveFn);
  auto f = exit_pbs_service.get_future();
  f.wait();
  server->Shutdown();
  serving_thread.join();
}

void run_server_for_testing_pinsketch_service_large_scale(size_t d, uint32_t start, unsigned seed);
void DoPinSketchServiceLargeScale(size_t d, uint32_t start = 1000, unsigned seed=20200717u);
void run_server_for_testing_pbs_service_large_scale(size_t d, uint32_t start, unsigned seed);
void DoParityBitmapSketchServiceLargeScale(size_t d, uint32_t start = 1000, unsigned seed=20200717u);
void run_server_for_testing_ddigest_service_large_scale(size_t d, uint32_t start, unsigned seed);
void DoDDigestServiceLargeScale(size_t d, uint32_t start = 1000, unsigned seed=20200717u);
void run_server_for_testing_graphene_service_large_scale(size_t d, uint32_t start, unsigned seed);
void DoGrapheneServiceLargeScale(size_t d, uint32_t start = 1000, unsigned seed=20200717u);

void run_server_for_testing_push_pull_service() {
  std::string server_address("0.0.0.0:50051");
  EstimationServiceImpl service;

  auto server_data_ptr = std::make_shared<tsl::ordered_map<Key, Value>>(
      tsl::ordered_map<Key, Value>{
          {4, "4444"}, {6, "666666"}, {3, "333"}, {5, "55555"}});

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
  std::cout << "Server (testing push and pull services) listening on "
            << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  auto serveFn = [&]() { server->Wait(); };

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
  std::this_thread::sleep_for(
      1s);  // make sure server is ready when client calls
  {
    // start a client
    std::string target_str = "localhost:50051";
    ReconciliationClient client(
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    tsl::ordered_map<Key, Value> client_data{
        {1, "1"}, {2, "22"}, {3, "333"}, {5, "55555"}};
    tsl::ordered_map<Key, Value> expected{{1, "1"},    {2, "22"},
                                          {3, "333"},  {5, "55555"},
                                          {4, "4444"}, {6, "666666"}};
    EXPECT_TRUE(client.Reconciliation_PinSketch(client_data, 4));

    EXPECT_EQ(expected.size(), client_data.size());
    for (const auto& kv: expected) {
      EXPECT_TRUE(client_data.count(kv.first) > 0);
      EXPECT_EQ(kv.second, client_data.at(kv.first));
    }

  }

  stop_pinsketch_service();
  th_run_server.join();
}

TEST(ReconciliationServicesTest, PinSketchServiceLargeScale) {
  // 100000 is too large for PinSketch
  for (auto d: {100, 1000, 10000})
    DoPinSketchServiceLargeScale(d);
}

TEST(ReconciliationServicesTest, ParityBitmapSketchServiceSmallScale) {
  std::thread th_run_server(run_server_for_testing_pbs_service);
  // make sure server is ready when client calls
  std::this_thread::sleep_for(1s);
  {
    // start a client
    std::string target_str = "localhost:50051";
    ReconciliationClient client(
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    tsl::ordered_map<Key, Value> client_data{
        {1, "1"}, {2, "22"}, {3, "333"}, {5, "55555"}};
    tsl::ordered_map<Key, Value> expected{{1, "1"},    {2, "22"},
                                          {3, "333"},  {5, "55555"},
                                          {4, "4444"}, {6, "666666"}};
    EXPECT_TRUE(client.Reconciliation_ParityBitmapSketch(client_data, 4));
    EXPECT_EQ(expected.size(), client_data.size());
    for (const auto& kv: expected) {
      EXPECT_TRUE(client_data.count(kv.first) > 0);
      EXPECT_EQ(kv.second, client_data.at(kv.first));
    }
  }

  stop_pbs_service();
  th_run_server.join();
}

TEST(ReconciliationServicesTest, ParityBitmapSketchServiceLargeScale) {
  for (auto d: {100, 1000, 10000, 100000})
    DoParityBitmapSketchServiceLargeScale(d);
}

TEST(ReconciliationServicesTest, DDigestService) {
  std::thread th_run_server(run_server_for_testing_ddigest_service);
  std::this_thread::sleep_for(
      1s);  // make sure server is ready when client calls
  {
    // start a client
    std::string target_str = "localhost:50051";
    ReconciliationClient client(
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    tsl::ordered_map<Key, Value> client_data{
        {1, "1"}, {2, "22"}, {3, "333"}, {5, "55555"}};
    tsl::ordered_map<Key, Value> expected{{1, "1"},    {2, "22"},
                                          {3, "333"},  {5, "55555"},
                                          {4, "4444"}, {6, "666666"}};
    EXPECT_TRUE(client.Reconciliation_DDigest(client_data, 4));

    EXPECT_EQ(expected.size(), client_data.size());
    for (const auto& kv: expected) {
      EXPECT_TRUE(client_data.count(kv.first) > 0);
      EXPECT_EQ(kv.second, client_data.at(kv.first));
    }

  }

  stop_ddigest_service();
  th_run_server.join();
}

TEST(ReconciliationServicesTest, GrapheneService) {
  std::thread th_run_server(run_server_for_testing_graphene_service);
  std::this_thread::sleep_for(
      1s);  // make sure server is ready when client calls
  {
    // start a client
    std::string target_str = "localhost:50051";
    ReconciliationClient client(
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    tsl::ordered_map<Key, Value> client_data{
        {1, "1"},    {2, "22"},
        {3, "333"},  {5, "55555"},
        {4, "4444"}, {6, "666666"}};

    EXPECT_TRUE(client.Reconciliation_Graphene(client_data));

    std::vector<Key> pull_keys{1,2};
    tsl::ordered_map<Key, Value> expected{{1, "1"},    {2, "22"}};
    tsl::ordered_map<Key, Value> obtained;
    client.Pull(pull_keys.begin(), pull_keys.end(), obtained);
    for (const auto& kv: expected) {
      EXPECT_TRUE(client_data.count(kv.first) > 0);
      EXPECT_EQ(kv.second, obtained.at(kv.first));
    }

  }

  stop_graphene_service();
  th_run_server.join();
}

TEST(ReconciliationServicesTest, GrapheneServiceLargeScale) {
//  DoGrapheneServiceLargeScale(100);
  for (auto d: {100, 1000, 10000, 100000})
    DoGrapheneServiceLargeScale(d);
}

TEST(ReconciliationServicesTest, DDigestServiceLargeScale) {
  for (auto d: {100, 1000, 10000})
    DoDDigestServiceLargeScale(d);
}

TEST(ReconciliationServicesTest, PushPullService) {
  std::thread th_run_server(run_server_for_testing_push_pull_service);
  // make sure server is ready when client calls
  std::this_thread::sleep_for(1s);
  std::string target_str = "localhost:50051";
  ReconciliationClient client(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  {
    // test pull
    tsl::ordered_set<Key> pull_keys{3, 4};
    tsl::ordered_map<Key, Value> expected{{3, "333"}, {4, "4444"}};
    tsl::ordered_map<Key, Value> pulled;
    EXPECT_TRUE(client.Pull(pull_keys.cbegin(), pull_keys.cend(), pulled));
    EXPECT_EQ(expected, pulled);
  }

  {
    // test push
    tsl::ordered_set<Key> push_keys{1, 2};
    tsl::ordered_map<Key, Value> client_data{
        {1, "1"}, {2, "22"}, {3, "333"}, {5, "55555"}};
    tsl::ordered_map<Key, Value> expected{{1, "1"}, {2, "22"}};
    EXPECT_TRUE(client.Push(push_keys.cbegin(), push_keys.cend(), client_data));
    tsl::ordered_map<Key, Value> pulled;
    EXPECT_TRUE(client.Pull(push_keys.cbegin(), push_keys.cend(), pulled));
    EXPECT_EQ(expected, pulled);
  }

  {
    // test push and pull
    tsl::ordered_set<Key> push_keys{7};
    tsl::ordered_set<Key> pull_keys{3, 5};
    tsl::ordered_map<Key, Value> client_data{{7, "7777777"}};
    tsl::ordered_map<Key, Value> expected{
        {7, "7777777"}, {3, "333"}, {5, "55555"}};
    EXPECT_TRUE(client.PushAndPull(push_keys.cbegin(), push_keys.cend(),
                                   pull_keys.cbegin(), pull_keys.cend(),
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


std::shared_ptr<tsl::ordered_map<Key, Value>>
generateKeyValuePairs(size_t d, uint32_t start, unsigned seed=202007u) {
  std::mt19937_64 gen(seed);
  std::uniform_int_distribution<> dist;
  auto server_data_ptr = std::make_shared<tsl::ordered_map<Key, Value>>();
  for (size_t i = 0;i < d;++ i) {
    server_data_ptr->insert({start + i, std::to_string(dist(gen))});
  }

  return server_data_ptr;
}

void run_server_for_testing_graphene_service_large_scale(size_t d, uint32_t start, unsigned seed) {
  std::string server_address("0.0.0.0:50051");
  EstimationServiceImpl service;

  auto server_data_ptr = generateKeyValuePairs(0, start, seed);
  service.set_key_value_pairs(server_data_ptr);

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
  std::cout << "Server (testing graphene service large scale) listening on " << server_address
            << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  auto serveFn = [&]() { server->Wait(); };

  std::thread serving_thread(serveFn);
  auto f = exit_graphene_service.get_future();
  f.wait();
  server->Shutdown();
  serving_thread.join();
}

void DoGrapheneServiceLargeScale(size_t d, uint32_t start, unsigned seed) {
  reset_graphene_service();
  std::thread th_run_server(run_server_for_testing_graphene_service_large_scale, d, start, seed);
  // make sure server is ready when client calls
  std::this_thread::sleep_for(1s);
  {
    // start a client
    std::string target_str = "localhost:50051";
    ReconciliationClient client(
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    tsl::ordered_map<Key, Value> client_data;
    std::shared_ptr<tsl::ordered_map<Key, Value>> expected = generateKeyValuePairs(d, start, seed);
    client_data = *expected;
    auto succeed = client.Reconciliation_Graphene(client_data);
    EXPECT_TRUE(succeed);
    std::vector<Key> pulled_keys;
    tsl::ordered_map<Key, Value> obtained;
    for (const auto& kv: client_data) pulled_keys.push_back(kv.first);
    client.Pull(pulled_keys.cbegin(), pulled_keys.cend(), obtained);
    EXPECT_EQ(expected->size(), obtained.size());
    for (const auto& kv: *expected) {
      EXPECT_TRUE(obtained.count(kv.first) > 0);
      EXPECT_EQ(kv.second, obtained.at(kv.first));
    }
  }

  stop_graphene_service();
  th_run_server.join();
}

void run_server_for_testing_ddigest_service_large_scale(size_t d, uint32_t start, unsigned seed) {
  std::string server_address("0.0.0.0:50051");
  EstimationServiceImpl service;

  auto server_data_ptr = generateKeyValuePairs(d, start, seed);
  service.set_key_value_pairs(server_data_ptr);
  service.set_estimated_diff(d);

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
  std::cout << "Server (testing ddigest service large scale) listening on " << server_address
            << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  auto serveFn = [&]() { server->Wait(); };

  std::thread serving_thread(serveFn);
  auto f = exit_ddigest_service.get_future();
  f.wait();
  server->Shutdown();
  serving_thread.join();
}

void DoDDigestServiceLargeScale(size_t d, uint32_t start, unsigned seed) {
  reset_ddigest_service();
  std::thread th_run_server(run_server_for_testing_ddigest_service_large_scale, d, start, seed);
  // make sure server is ready when client calls
  std::this_thread::sleep_for(1s);
  {
    // start a client
    std::string target_str = "localhost:50051";
    ReconciliationClient client(
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    tsl::ordered_map<Key, Value> client_data;
    std::shared_ptr<tsl::ordered_map<Key, Value>> expected = generateKeyValuePairs(d, start, seed);
    auto succeed = client.Reconciliation_PinSketch(client_data, d);
    EXPECT_TRUE(succeed);
    EXPECT_EQ(expected->size(), client_data.size());
    for (const auto& kv: *expected) {
      EXPECT_TRUE(client_data.count(kv.first) > 0);
      EXPECT_EQ(kv.second, client_data.at(kv.first));
    }
  }

  stop_ddigest_service();
  th_run_server.join();
}

void run_server_for_testing_pinsketch_service_large_scale(size_t d, uint32_t start, unsigned seed) {
  std::string server_address("0.0.0.0:50051");
  EstimationServiceImpl service;

  auto server_data_ptr = generateKeyValuePairs(d, start, seed);
  service.set_key_value_pairs(server_data_ptr);
  service.set_estimated_diff(d);

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
  std::cout << "Server (testing pinsketch service large scale) listening on " << server_address
            << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  auto serveFn = [&]() { server->Wait(); };

  std::thread serving_thread(serveFn);
  auto f = exit_pinsketch_service.get_future();
  f.wait();
  server->Shutdown();
  serving_thread.join();
}

void DoPinSketchServiceLargeScale(size_t d, uint32_t start, unsigned seed) {
  reset_pinsketch_service();
  std::thread th_run_server(run_server_for_testing_pinsketch_service_large_scale, d, start, seed);
  // make sure server is ready when client calls
  std::this_thread::sleep_for(1s);
  {
    // start a client
    std::string target_str = "localhost:50051";
    ReconciliationClient client(
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    tsl::ordered_map<Key, Value> client_data;
    std::shared_ptr<tsl::ordered_map<Key, Value>> expected = generateKeyValuePairs(d, start, seed);
    auto succeed = client.Reconciliation_PinSketch(client_data, d);
    EXPECT_TRUE(succeed);
    EXPECT_EQ(expected->size(), client_data.size());
    for (const auto& kv: *expected) {
      EXPECT_TRUE(client_data.count(kv.first) > 0);
      EXPECT_EQ(kv.second, client_data.at(kv.first));
    }
  }

  stop_pinsketch_service();
  th_run_server.join();
}

void run_server_for_testing_pbs_service_large_scale(size_t d, uint32_t start, unsigned seed) {
  std::string server_address("0.0.0.0:50051");
  EstimationServiceImpl service;

  auto server_data_ptr = generateKeyValuePairs(d, start, seed);
  service.set_key_value_pairs(server_data_ptr);
  service.set_estimated_diff(d);

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
  std::cout << "Server (testing pbs service large scale) listening on " << server_address
            << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  auto serveFn = [&]() { server->Wait(); };

  std::thread serving_thread(serveFn);
  auto f = exit_pbs_service.get_future();
  f.wait();
  server->Shutdown();
  serving_thread.join();
}

void DoParityBitmapSketchServiceLargeScale(size_t d, uint32_t start, unsigned seed) {
  reset_pbs_service();
  std::thread th_run_server(run_server_for_testing_pbs_service_large_scale, d, start, seed);
  // make sure server is ready when client calls
  std::this_thread::sleep_for(1s);
  {
    // start a client
    std::string target_str = "localhost:50051";
    ReconciliationClient client(
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    tsl::ordered_map<Key, Value> client_data;
    std::shared_ptr<tsl::ordered_map<Key, Value>> expected = generateKeyValuePairs(d, start, seed);
    auto succeed = client.Reconciliation_ParityBitmapSketch(client_data, d);
    EXPECT_TRUE(succeed);
    EXPECT_EQ(expected->size(), client_data.size());
    for (const auto& kv: *expected) {
      EXPECT_TRUE(client_data.count(kv.first) > 0);
      EXPECT_EQ(kv.second, client_data.at(kv.first));
    }
  }

  stop_pbs_service();
  th_run_server.join();
}
#include <CLI/CLI.hpp>
#include "reconciliation_server.h"

int main(int argc, char **argv) {
  CLI::App app{"Set Reconciliation Benchmark Server"};
  std::string server_address = "0.0.0.0:50051";
  app.add_option("--address", server_address, "IP:port");

  CLI11_PARSE(app, argc, argv);

  EstimationServiceImpl service;
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
  std::cout << "Server listening on "
            << server_address << std::endl;
  server->Wait();
  return 0;
}
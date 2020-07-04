#include <memory>
#include <iostream>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "reconciliation.grpc.pb.h"
#include <tow.h>
#include <xxhash_wrapper.h>


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using reconciliation::EstimateRequest;
using reconciliation::EstimateReply;
using reconciliation::Estimation;

constexpr unsigned DEFAULT_SKETCHES_ = 128;
constexpr unsigned DEFAULT_SEED = 142857;

// Logic and data behind the server's behavior.
class EstimationServiceImpl final : public Estimation::Service {
  Status Estimate(ServerContext *context, const EstimateRequest *request,
                  EstimateReply *reply) override {
    double d = 0.0, tmp;

    for (size_t i = 0; i < _sketches.size(); ++i) {
      tmp = _sketches[i] - request->sketches(i);
      d += tmp * tmp;
    }
    reply->set_estimated_value(static_cast<float>(d / _estimator.num_sketches()));
    return Status::OK;
  }
 public:
  EstimationServiceImpl() : Estimation::Service(), _estimator(DEFAULT_SKETCHES_, DEFAULT_SEED) {}
//  EstimationServiceImpl(unsigned num_sketches, unsigned seed) : Estimation::Service(), _estimator(num_sketches, seed) {}

  // local computation
  template<typename Iterator>
  void LocalSketchFor(Iterator first, Iterator last) {
    _sketches = _estimator.apply(first, last);
  }
 private:
  TugOfWarHash<XXHash> _estimator;
  std::vector<int> _sketches;
};

void RunServer() {
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
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char **argv) {
  RunServer();
  return 0;
}
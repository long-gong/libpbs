#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <grpcpp/grpcpp.h>

#include "reconciliation.grpc.pb.h"
#include <tow.h>
#include <xxhash_wrapper.h>
#include <random.h>

using namespace std::chrono_literals;
constexpr unsigned DEFAULT_SKETCHES_ = 128;
constexpr unsigned DEFAULT_SEED = 142857;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using reconciliation::EstimateRequest;
using reconciliation::EstimateReply;
using reconciliation::Estimation;

class ReconciliationClient {
 public:
  ReconciliationClient(std::shared_ptr<Channel> channel)
      : stub_(Estimation::NewStub(channel)), _estimator(DEFAULT_SKETCHES_, DEFAULT_SEED) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  template<typename Iterator>
  float Estimation(Iterator first, Iterator last) {

    auto sketches_ = _estimator.apply(first, last);
    // Data we are sending to the server.
    EstimateRequest request;
    request.mutable_sketches()->Add(sketches_.begin(), sketches_.end());

    // Container for the data we expect from the server.
    EstimateReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Estimate(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.estimated_value();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return -1;
    }
  }

 private:
  std::unique_ptr<Estimation::Stub> stub_;
  TugOfWarHash<XXHash> _estimator;
};

int main(int argc, char **argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint specified by
  // the argument "--target=" which is the only expected argument.
  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  std::string target_str;
  std::string arg_str("--target");
  if (argc > 1) {
    std::string arg_val = argv[1];
    size_t start_pos = arg_val.find(arg_str);
    if (start_pos != std::string::npos) {
      start_pos += arg_str.size();
      if (arg_val[start_pos] == '=') {
        target_str = arg_val.substr(start_pos + 1);
      } else {
        std::cout << "The only correct argument syntax is --target=" << std::endl;
        return 0;
      }
    } else {
      std::cout << "The only acceptable argument is --target=" << std::endl;
      return 0;
    }
  } else {
    target_str = "localhost:50051";
  }

  ReconciliationClient reconciliation_client(grpc::CreateChannel(
      target_str, grpc::InsecureChannelCredentials()));
  for (size_t n = 8; n < 4096; n *= 2) {
    auto testset = GenerateRandomSet32(n);
    assert(testset.size() == n);
    auto reply = reconciliation_client.Estimation(testset.begin(), testset.end());
    std::cout << "Estimate :  " << reply << " (" << n << ")" << std::endl;
    std::this_thread::sleep_for(5s);
  }
  // Sample results:
//  Estimate :  9.59375 (8)
//  Estimate :  17.5 (16)
//  Estimate :  26.2812 (32)
//  Estimate :  59.9062 (64)
//  Estimate :  133.688 (128)
//  Estimate :  270.562 (256)
//  Estimate :  573.125 (512)
//  Estimate :  923.531 (1024)
//  Estimate :  1827.06 (2048)

  return 0;
}

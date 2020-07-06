#ifndef RECONCILIATION_CLIENT_H_
#define RECONCILIATION_CLIENT_H_

#include <grpcpp/grpcpp.h>
#include <random.h>
#include <tow.h>
#include <tsl/ordered_map.h>
#include <tsl/ordered_set.h>
#include <xxhash_wrapper.h>

#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <thread>

#include "constants.h"
#include "pinsketch.h"
#include "pbs.h"
#include "reconciliation.grpc.pb.h"

using namespace std::chrono_literals;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using reconciliation::EstimateReply;
using reconciliation::EstimateRequest;
using reconciliation::Estimation;

using reconciliation::KeyValue;
using reconciliation::PinSketchRequest;
using reconciliation::PinSketchReply;

using reconciliation::SynchronizeMessage;

// template<typename MyHash=XXHASH>
class ReconciliationClient {
 public:
  ReconciliationClient(std::shared_ptr<Channel> channel)
      : stub_(Estimation::NewStub(channel)),
        _estimator(DEFAULT_SKETCHES_, DEFAULT_SEED) {}

  template<typename PushKeyIterator, typename PullKeyIterator>
  bool PushAndPull(PushKeyIterator push_first,
                   PushKeyIterator push_last,
                   PullKeyIterator pull_first,
                   PullKeyIterator pull_last,
                   tsl::ordered_map<Key, Value> &key_value_pairs) {
    SynchronizeMessage syn_req;
    for (auto it = pull_first; it != pull_last; ++it) {
      if (key_value_pairs.contains(*it)) return false;
      syn_req.mutable_pulls()->Add(*it);
    }
    for (auto it = push_first; it != push_last; ++it) {
      if (!key_value_pairs.contains(*it)) return false;
      auto kv = syn_req.mutable_pushes()->Add();
      kv->set_key(*it);
      kv->set_value(key_value_pairs[*it]);
    }
    SynchronizeMessage syn_reply;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext syn_context;
    // The actual RPC.
    Status syn_status = stub_->Synchronize(&syn_context, syn_req, &syn_reply);
    if (!syn_status.ok()) {
      std::cerr << (std::to_string(syn_status.error_code()) + ": " +
          syn_status.error_message())
                << std::endl;
      return false;
    }
    for (const auto &kv: syn_reply.pushes())
      key_value_pairs.insert({kv.key(), kv.value()});
    return true;
  }

  template<typename KeyIterator>
  bool Pull(KeyIterator first, KeyIterator last, tsl::ordered_map<Key, Value> &key_value_pairs) {
    SynchronizeMessage syn_req;
    for (auto it = first; it != last; ++it) {
      if (key_value_pairs.contains(*it)) return false;
      syn_req.mutable_pulls()->Add(*it);
    }
    SynchronizeMessage syn_reply;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext syn_context;
    // The actual RPC.
    Status syn_status = stub_->Synchronize(&syn_context, syn_req, &syn_reply);
    if (!syn_status.ok()) {
      std::cerr << (std::to_string(syn_status.error_code()) + ": " +
          syn_status.error_message())
                << std::endl;
      return false;
    }
    for (const auto &kv: syn_reply.pushes())
      key_value_pairs.insert({kv.key(), kv.value()});
    return true;
  }

  template<typename KeyIterator>
  bool Push(KeyIterator first, KeyIterator last, const tsl::ordered_map<Key, Value> &key_value_pairs) {
    SynchronizeMessage syn_req;
    for (auto it = first; it != last; ++it) {
      if (!key_value_pairs.contains(*it)) return false;
      auto kv = syn_req.mutable_pushes()->Add();
      kv->set_key(*it);
      kv->set_value(key_value_pairs.at(*it));
    }
    SynchronizeMessage syn_reply;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext syn_context;
    // The actual RPC.
    Status syn_status = stub_->Synchronize(&syn_context, syn_req, &syn_reply);
    if (!syn_status.ok()) {
      std::cerr << (std::to_string(syn_status.error_code()) + ": " +
          syn_status.error_message())
                << std::endl;
      return false;
    }
    return true;
  }

  // template<typename Key, typename Value>
  bool Reconciliation_PinSketch(tsl::ordered_map<Key, Value> &key_value_pairs,
                                ssize_t d = -1) {
    size_t scaled_d = d;
    if (d == -1) {
      auto est = EstimationKeyValuePairs(key_value_pairs.cbegin(),
                                         key_value_pairs.cend());
      scaled_d = ESTIMATE_SM99(est);
    }

    PinSketchRequest request;
    PinSketch ps(sizeof(Key) * BITS_IN_ONE_BYTE /* signature size in bits */,
                 scaled_d /* error-correcting capacity */);
    request.set_sketch(ps.encode_and_serialize_key_value_pairs(
        key_value_pairs.cbegin(), key_value_pairs.cend()));

    PinSketchReply reply;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;
    // The actual RPC.
    Status status = stub_->ReconcilePinSketch(&context, request, &reply);
    // Act upon its status.
    if (!status.ok()) {
      std::cerr << (std::to_string(status.error_code()) + ": " +
          status.error_message())
                << std::endl;
      return false;
    }
    for (const auto &kv : reply.pushed_key_values()) {
      if (key_value_pairs.contains(kv.key())) return false;
      key_value_pairs.insert({kv.key(), kv.value()});
    }
    if (!reply.missing_keys().empty())
      return Push(reply.missing_keys().cbegin(), reply.missing_keys().cend(), key_value_pairs);
    return true;
  }

  template<typename Iterator>
  float EstimationKeyValuePairs(Iterator first, Iterator last) {
    auto sketches_ = _estimator.apply_key_value_pairs(first, last);
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

#endif // RECONCILIATION_CLIENT_H_
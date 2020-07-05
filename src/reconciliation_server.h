#ifndef RECONCILIATION_SERVER_H_
#define RECONCILIATION_SERVER_H_

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <tsl/ordered_map.h>
#include <tsl/ordered_set.h>

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "tow.h"
#include "xxhash_wrapper.h"
#include "pinsketch.h"
#include "reconciliation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;

using reconciliation::EstimateReply;
using reconciliation::EstimateRequest;
using reconciliation::Estimation;

using reconciliation::PinSketchReply;
using reconciliation::PinSketchRequest;

using reconciliation::KeyValue;
using reconciliation::SynchronizeMessage;

#include "constants.h"

// Logic and data behind the server's behavior.
// template<typename Key=int, typename Value=std::string>
class EstimationServiceImpl final : public Estimation::Service {

  Status Estimate(ServerContext *context, const EstimateRequest *request,
                  EstimateReply *reply) override {
    double d = 0.0, tmp;
    if (_sketches.empty()) {
      if (_key_value_pairs == nullptr) {
        return Status(StatusCode::UNAVAILABLE, "Server seems not ready yet");
      }
      LocalSketchForKeyValuePairs(_key_value_pairs->cbegin(), _key_value_pairs->cend());
    }

    for (size_t i = 0; i < _sketches.size(); ++i) {
      tmp = _sketches[i] - request->sketches(i);
      d += tmp * tmp;
    }

    reply->set_estimated_value(
        static_cast<float>(d / _estimator.num_sketches()));

    _estimated_diff =
        static_cast<ssize_t>(INFLATION_RATIO * reply->estimated_value());
    return Status::OK;
  }

  Status Synchronize(ServerContext *context, const SynchronizeMessage *request,
                     SynchronizeMessage *response) override {
    if (_key_value_pairs == nullptr) {
      return Status(StatusCode::UNAVAILABLE, "Server seems not ready yet");
    }

    for (const auto &kv : request->pushes()) {
      Key key = static_cast<Key>(kv.key());
      _key_value_pairs->insert({key, kv.value()});
    }

    for (const auto &key : request->pulls()) {
      Key key_ = static_cast<Key>(key);
      if (!(_key_value_pairs->contains(key_))) {
        return Status(StatusCode::NOT_FOUND,
                      "Some required keys can not be found in the key-value "
                      "pairs were found on server side");
      }
      auto kv = response->mutable_pushes()->Add();
      kv->set_key(key);
      kv->set_value((*_key_value_pairs)[key_]);
    }
    return Status::OK;
  }

  Status ReconcilePinSketch(ServerContext *context,
                            const PinSketchRequest *request,
                            PinSketchReply *response) {
    if (_estimated_diff == -1 )
      return Status(StatusCode::UNAVAILABLE, "Please call Estimate() first");
    if (_key_value_pairs == nullptr)
      return Status(StatusCode::UNAVAILABLE, "Server seems not ready yet");


    PinSketch ps(sizeof(Key) * BITS_IN_ONE_BYTE, _estimated_diff);
    ps.encode_key_value_pairs(_key_value_pairs->cbegin(),
                              _key_value_pairs->cend());

    std::vector<uint64_t> differences;
    bool succeed =
        ps.decode((unsigned char *)&(request->sketch()[0]), differences);
    if (!succeed) {
      // do something
    }

    for (const auto key : differences) {
      Key key_ = static_cast<Key>(key);
      if (_key_value_pairs->contains(key)) {
        auto kv = response->mutable_pushed_key_values()->Add();
        kv->set_key(key_);
        kv->set_value((*_key_value_pairs)[key_]);
      } else {
        response->mutable_missing_keys()->Add(key_);
      }
    }


    return Status::OK;
  }

 public:
  EstimationServiceImpl()
      : Estimation::Service(),
        _estimator(DEFAULT_SKETCHES_, DEFAULT_SEED),
        _estimated_diff(-1),
        _key_value_pairs(nullptr) {}


  void set_key_value_pairs(
      const std::shared_ptr<tsl::ordered_map<Key, Value>>& other) {
    _key_value_pairs = other;
  }

  void set_estimated_diff(size_t d) { _estimated_diff = d; }

  ssize_t estimated_diff() const { return _estimated_diff; }

  const tsl::ordered_map<Key, Value> *key_value_pairs() const {
    return _key_value_pairs.get();
  }

  template <typename Iterator>
  void LocalSketchFor(Iterator first, Iterator last) {
    _sketches = _estimator.apply(first, last);
  }

  template <typename Iterator>
  void LocalSketchForKeyValuePairs(Iterator first, Iterator last) {
    _sketches = _estimator.apply_key_value_pairs(first, last);
  }

 private:
  TugOfWarHash<XXHash> _estimator;
  std::vector<int> _sketches;

  ssize_t _estimated_diff;
  std::shared_ptr<tsl::ordered_map<Key, Value>> _key_value_pairs;
};

#endif // RECONCILIATION_SERVER_H_
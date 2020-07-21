#ifndef RECONCILIATION_SERVER_H_
#define RECONCILIATION_SERVER_H_

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <iblt/iblt.h>
#include <iblt/utilstrencodings.h>
#include <iblt/search_params.h>
#include <bloom/bloom_filter.h>
#include <tsl/ordered_map.h>
#include <tsl/ordered_set.h>

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "pbs.h"
#include "pinsketch.h"
#include "reconciliation.grpc.pb.h"
#include "tow.h"
#include "xxhash_wrapper.h"

#include "bench_utils.h"
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;

using reconciliation::SetUpRequest;
using reconciliation::SetUpReply;
using reconciliation::SetUpRequest_Method_DDigest;
using reconciliation::SetUpRequest_Method_PinSketch;
using reconciliation::SetUpRequest_Method_Graphene;
using reconciliation::SetUpRequest_Method_PBS;
using reconciliation::SetUpRequest_Method_END;
using reconciliation::SetUpReply_PreviousExperimentStatus_SUCCEED;
using reconciliation::SetUpReply_PreviousExperimentStatus_FAILED;
using reconciliation::SetUpReply_PreviousExperimentStatus_NA;

using reconciliation::EstimateReply;
using reconciliation::EstimateRequest;
using reconciliation::Estimation;

using reconciliation::PinSketchReply;
using reconciliation::PinSketchRequest;

using reconciliation::IbfCell;
using reconciliation::DDigestReply;
using reconciliation::DDigestRequest;

using reconciliation::GrapheneReply;
using reconciliation::GrapheneRequest;

using reconciliation::PbsReply;
using reconciliation::PbsRequest;

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
      LocalSketchForKeyValuePairs(_key_value_pairs->cbegin(),
                                  _key_value_pairs->cend());
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

  Status ReconcileSetUp(ServerContext *context, const SetUpRequest *request,
                        SetUpReply *response) override {
    auto alg = request->next_algorithm();
    auto d = request->d();
    auto usz = request->usz();
    auto seed = request->seed();
    auto value_size = request->object_sz();

    switch (alg) {
      case SetUpRequest_Method_DDigest:
      case SetUpRequest_Method_PinSketch:
      {
        if (_key_value_pairs == nullptr) _key_value_pairs = std::make_shared<tsl::ordered_map<Key, Value>>();
        only_for_benchmark::GenerateKeyValuePairs<tsl::ordered_map<Key, Value>, Key>(*_key_value_pairs, usz, value_size, seed);
        LocalSketchForKeyValuePairs(_key_value_pairs->cbegin(), _key_value_pairs->cend());
        response->set_status(reconciliation::SetUpReply_PreviousExperimentStatus_NA);
        return Status::OK;
      }
      case SetUpRequest_Method_Graphene:
      case SetUpRequest_Method_PBS:
      {
        if (_key_value_pairs == nullptr) _key_value_pairs = std::make_shared<tsl::ordered_map<Key, Value>>();
        only_for_benchmark::GenerateKeyValuePairs<tsl::ordered_map<Key, Value>, Key>(*_key_value_pairs, usz, value_size, seed);
        for (auto it = _key_value_pairs->begin();it != _key_value_pairs->begin() + d;++ it)
          _key_value_pairs->erase(it);
        LocalSketchForKeyValuePairs(_key_value_pairs->cbegin(), _key_value_pairs->cend());
        response->set_status(reconciliation::SetUpReply_PreviousExperimentStatus_NA);
        return Status::OK;
      }
      case SetUpRequest_Method_END:
      {
        if (_key_value_pairs == nullptr) {
          response->set_status(reconciliation::SetUpReply_PreviousExperimentStatus_FAILED);
        } else {
          tsl::ordered_map<Key, Value> ground_truth;
          only_for_benchmark::GenerateKeyValuePairs<tsl::ordered_map<Key, Value>, Key>(ground_truth,
                                                                                       usz,
                                                                                       value_size,
                                                                                       seed);
          if (only_for_benchmark::is_equal(*_key_value_pairs, ground_truth)) {
            response->set_status(reconciliation::SetUpReply_PreviousExperimentStatus_SUCCEED);
          } else {
            response->set_status(reconciliation::SetUpReply_PreviousExperimentStatus_FAILED);
          }
          _key_value_pairs->clear();
        }
        return Status::OK;
      }
      default:
        return Status::OK;
    }

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

  Status ReconcileGraphene(ServerContext *context, const GrapheneRequest *request,
                           GrapheneReply *response) override {
    if (_key_value_pairs == nullptr)
      return Status(StatusCode::UNAVAILABLE, "Server seems not ready yet");

    auto setasize = request->m();
    auto setbsize = _key_value_pairs->size();
    const double DEFAULT_CB = (1.0 - 239.0 / 240);
    const std::vector<uint8_t> DUMMY_VAL{0};
    const int VAL_SIZE = 1; // bytes
    struct search_params params;
    bloom_parameters bf_params;
    double a, fpr_sender;
    int iblt_rows_first;
    params.CB_solve_a(setasize /* mempool_size */, setbsize /* blk_size */,
                      setbsize /* blk_size */, 0, DEFAULT_CB, a, fpr_sender,
                      iblt_rows_first);
    a = std::ceil(a);

    response->set_a(int(a));
    // create an IBLT
    IBLT iblt_sender_first = IBLT(int(a), VAL_SIZE);

    bool no_bf = (std::abs(1.0 - fpr_sender) < CONSIDER_TOBE_ZERO);
    // create a Bloom filter
    if (!no_bf) {
      bf_params.projected_element_count = (setbsize == 0 ? 1 : setbsize);
      bf_params.false_positive_probability = fpr_sender;
      bf_params.compute_optimal_parameters();
      bloom_filter bloom_sender = bloom_filter(bf_params);
      for (const auto &kv : *_key_value_pairs) {
        bloom_sender.insert(kv.first);
        iblt_sender_first.insert(kv.first /* key */, DUMMY_VAL /* (dummy) value */);
      }
      auto ssz = bloom_sender.size() / 8;
      response->mutable_bf()->resize(ssz, 0);
      std::copy(bloom_sender.table(), bloom_sender.table() + ssz, response->mutable_bf()->begin());
      response->set_n(bf_params.projected_element_count);
      response->set_fpr(fpr_sender);
    } else {
      for (const auto &kv : *_key_value_pairs) {
        iblt_sender_first.insert(kv.first /* key */, DUMMY_VAL /* (dummy) value */);
      }
    }
    auto table = iblt_sender_first.data();
    for (const auto &cell: table) {
      auto new_cell = response->mutable_ibf()->Add();
      new_cell->set_count(cell.count);
      new_cell->set_keysum(cell.keySum);
      new_cell->set_keycheck(cell.keyCheck);
    }
    return Status::OK;
  }

  Status ReconcileDDigest(ServerContext *context, const DDigestRequest *request,
                          DDigestReply *response) override {
    if (_estimated_diff == -1)
      return Status(StatusCode::UNAVAILABLE, "Please call Estimate() first");
    if (_key_value_pairs == nullptr)
      return Status(StatusCode::UNAVAILABLE, "Server seems not ready yet");

    const int VAL_SIZE = 1;  // bytes
    const std::vector<uint8_t> VAL{0};
    float HEDGE =
        2.0;  // use suggested value provided in what's difference paper
    IBLT my_iblt(_estimated_diff, VAL_SIZE, HEDGE,
                 (_estimated_diff > 200 ? 3 : 4));
    for (const auto &kv : *_key_value_pairs) {
      my_iblt.insert(kv.first, VAL);
    }

    IBLT other_iblt(_estimated_diff, VAL_SIZE, HEDGE,
                    (_estimated_diff > 200 ? 3 : 4));

    using my_iterator_t = decltype(request->cells().cbegin());
    other_iblt.set(request->cells().cbegin(),
                   request->cells().cend(),
                   [](my_iterator_t it) { return it->count(); },
                   [](my_iterator_t it) { return it->keysum(); },
                   [](my_iterator_t it) { return it->keycheck(); });

    std::set<std::pair<uint64_t, std::vector<uint8_t>>> pos, neg;
    auto ibltD = my_iblt - other_iblt;
    bool succeed = ibltD.listEntries(pos, neg);

    response->set_succeed(succeed);

    if (succeed) {
      for (const auto &key : neg) {
        Key key_ = static_cast<Key>(key.first);
        response->mutable_missing_keys()->Add(key_);
      }
      for (const auto &key : pos) {
        Key key_ = static_cast<Key>(key.first);
        auto kv = response->mutable_pushed_key_values()->Add();

        if (!_key_value_pairs->contains(key_)) {
          response->set_succeed(false);
          break;
        }
        kv->set_key(key_);
        kv->set_value((*_key_value_pairs)[key_]);
      }
    }

    return Status::OK;
  }

  Status ReconcilePinSketch(ServerContext *context,
                            const PinSketchRequest *request,
                            PinSketchReply *response) override {
    if (_estimated_diff == -1)
      return Status(StatusCode::UNAVAILABLE, "Please call Estimate() first");
    if (_key_value_pairs == nullptr)
      return Status(StatusCode::UNAVAILABLE, "Server seems not ready yet");

    PinSketch ps(sizeof(Key) * BITS_IN_ONE_BYTE, _estimated_diff);
    ps.encode_key_value_pairs(_key_value_pairs->cbegin(),
                              _key_value_pairs->cend());

    std::vector<uint64_t> differences;
    bool succeed =
        ps.decode((unsigned char *) &(request->sketch()[0]), differences);
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

  Status ReconcileParityBitmapSketch(ServerContext *context,
                                     const PbsRequest *request,
                                     PbsReply *response) override {
    if (_estimated_diff == -1)
      return Status(StatusCode::UNAVAILABLE, "Please call Estimate() first");
    if (_key_value_pairs == nullptr)
      return Status(StatusCode::UNAVAILABLE, "Server seems not ready yet");

    std::vector<uint64_t> xors, checksums;
    std::shared_ptr<libpbs::PbsEncodingMessage> my_enc;

    if (_pbs == nullptr) {
      _pbs = std::make_unique<libpbs::ParityBitmapSketch>(_estimated_diff);
      for (const auto &kv : *_key_value_pairs) {
        _pbs->add(kv.first);
      }
      if (!request->encoding_hint().empty()) {
        throw std::runtime_error(
            "encoding hint in the first round should be empty!!");
      }
      auto[my_enc_tmp, dummy] = _pbs->encode();
      (void) dummy;  // avoid unused variable warning
      my_enc = my_enc_tmp;
    } else {
      libpbs::PbsEncodingHintMessage hint(_pbs->hint_max_range());
      hint.parse((const uint8_t *) request->encoding_hint().c_str(),
                 request->encoding_hint().size());
      my_enc = _pbs->encodeWithHint(hint);
    }

    libpbs::PbsEncodingMessage other_enc(my_enc->field_sz, my_enc->capacity,
                                         my_enc->num_groups);
    other_enc.parse((const uint8_t *) request->encoding_msg().c_str(),
                    request->encoding_msg().size());
    auto decoding_msg = _pbs->decode(other_enc, xors, checksums);

    auto ssz = decoding_msg->serializedSize();
    response->mutable_decoding_msg()->resize(ssz, 0);

    decoding_msg->write((uint8_t *) &(*response->mutable_decoding_msg())[0]);

    for (auto xor_each : xors) *(response->mutable_xors()->Add()) = xor_each;

    std::vector<uint64_t> tests;
    tests.insert(tests.end(), response->xors().cbegin(),
                 response->xors().cend());

    for (auto checksum : checksums)
      *(response->mutable_checksum()->Add()) = checksum;

    if (!request->pushed_key_values().empty()) {
      for (const auto &kv : request->pushed_key_values()) {
        if (_key_value_pairs->contains(kv.key()))
          return Status(StatusCode::ALREADY_EXISTS, "duplications");
        _key_value_pairs->insert({kv.key(), kv.value()});
      }
    }

    if (!request->missing_keys().empty()) {
      for (auto k : request->missing_keys()) {
        if (!_key_value_pairs->contains(k))
          return Status(StatusCode::NOT_FOUND, "fake");
        auto kv = response->mutable_pushed_key_values()->Add();
        kv->set_key(k);
        kv->set_value(_key_value_pairs->at(k));
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
      const std::shared_ptr<tsl::ordered_map<Key, Value>> &other) {
    _key_value_pairs = other;
  }

  void set_estimated_diff(size_t d) { _estimated_diff = d; }

  [[nodiscard]] ssize_t estimated_diff() const { return _estimated_diff; }

  [[nodiscard]] const tsl::ordered_map<Key, Value> *key_value_pairs() const {
    return _key_value_pairs.get();
  }

  template<typename Iterator>
  void LocalSketchFor(Iterator first, Iterator last) {
    _sketches = _estimator.apply(first, last);
  }

  template<typename Iterator>
  void LocalSketchForKeyValuePairs(Iterator first, Iterator last) {
    _sketches = _estimator.apply_key_value_pairs(first, last);
  }

 private:
  TugOfWarHash<XXHash> _estimator;
  std::vector<int> _sketches;

  ssize_t _estimated_diff;
  std::shared_ptr<tsl::ordered_map<Key, Value>> _key_value_pairs;

  std::unique_ptr<libpbs::ParityBitmapSketch> _pbs{};
};

#endif  // RECONCILIATION_SERVER_H_
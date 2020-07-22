#ifndef RECONCILIATION_CLIENT_H_
#define RECONCILIATION_CLIENT_H_

#include <bloom/bloom_filter.h>
#include <fmt/format.h>
#include <grpcpp/grpcpp.h>
#include <iblt/iblt.h>
#include <iblt/search_params.h>
#include <iblt/utilstrencodings.h>
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

#include "SimpleTimer.h"
#include "bench_utils.h"
#include "constants.h"
#include "pbs.h"
#include "pinsketch.h"
#include "reconciliation.grpc.pb.h"

using namespace std::chrono_literals;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using reconciliation::SetUpReply;
using reconciliation::SetUpReply_PreviousExperimentStatus_SUCCEED;
using reconciliation::SetUpRequest;
using reconciliation::SetUpRequest_Method_DDigest;
using reconciliation::SetUpRequest_Method_END;
using reconciliation::SetUpRequest_Method_Graphene;
using reconciliation::SetUpRequest_Method_PBS;
using reconciliation::SetUpRequest_Method_PinSketch;

using reconciliation::EstimateReply;
using reconciliation::EstimateRequest;
using reconciliation::Estimation;

using reconciliation::KeyValue;
using reconciliation::PinSketchReply;
using reconciliation::PinSketchRequest;

using reconciliation::DDigestReply;
using reconciliation::DDigestRequest;
using reconciliation::IbfCell;

using reconciliation::GrapheneReply;
using reconciliation::GrapheneRequest;

using reconciliation::PbsReply;
using reconciliation::PbsRequest;

using reconciliation::SynchronizeMessage;

namespace {
constexpr unsigned PBS_MAX_ROUNDS = 3;
}
// template<typename MyHash=XXHASH>
class ReconciliationClient {
 public:
  ReconciliationClient(std::shared_ptr<Channel> channel)
      : stub_(Estimation::NewStub(channel)),
        _estimator(DEFAULT_SKETCHES_, DEFAULT_SEED) {}

  template <typename PushKeyIterator, typename PullKeyIterator>
  bool PushAndPull(PushKeyIterator push_first, PushKeyIterator push_last,
                   PullKeyIterator pull_first, PullKeyIterator pull_last,
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
    for (const auto &kv : syn_reply.pushes())
      key_value_pairs.insert({kv.key(), kv.value()});
    return true;
  }

  template <typename KeyIterator>
  bool Pull(KeyIterator first, KeyIterator last,
            tsl::ordered_map<Key, Value> &key_value_pairs) {
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
    for (const auto &kv : syn_reply.pushes())
      key_value_pairs.insert({kv.key(), kv.value()});
    return true;
  }

  template <typename KeyIterator>
  bool Push(KeyIterator first, KeyIterator last,
            const tsl::ordered_map<Key, Value> &key_value_pairs) {
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

  bool verifyServerSide(size_t usz, size_t value_sz, unsigned exp_seed) {
    SetUpRequest request;
    request.set_seed(exp_seed);
    request.set_usz(usz);
    request.set_d(0);
    request.set_next_algorithm(SetUpRequest_Method_END);
    request.set_object_sz(value_sz);
    SetUpReply reply;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;
    // The actual RPC.
    Status status = stub_->ReconcileSetUp(&context, request, &reply);
    return reply.status() == SetUpReply_PreviousExperimentStatus_SUCCEED;
  }

  bool SetUp_PinSketch(size_t usz, size_t d, size_t value_sz, unsigned exp_seed,
                       double &completed_time) {
    SetUpRequest request;
    request.set_seed(exp_seed);
    request.set_d(d);
    request.set_usz(usz);
    request.set_next_algorithm(SetUpRequest_Method_PinSketch);
    request.set_object_sz(value_sz);
    SetUpReply reply;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;
    // The actual RPC.
    Status status = stub_->ReconcileSetUp(&context, request, &reply);

    tsl::ordered_map<Key, Value> key_value_pairs;
    only_for_benchmark::GenerateKeyValuePairs<tsl::ordered_map<Key, Value>,
                                              Key>(key_value_pairs, usz,
                                                   value_sz, exp_seed);
    //
    key_value_pairs.erase(key_value_pairs.cbegin(),
                          key_value_pairs.cbegin() + d);
    only_for_benchmark::SimpleTimer timer;
    completed_time = 0;
    timer.restart();
    auto succeed = Reconciliation_PinSketch(key_value_pairs);
    completed_time = timer.elapsed();
    if (!succeed) return false;

    tsl::ordered_map<Key, Value> ground_truth;
    only_for_benchmark::GenerateKeyValuePairs<tsl::ordered_map<Key, Value>,
                                              Key>(ground_truth, usz, value_sz,
                                                   exp_seed);
    return only_for_benchmark::is_equal(ground_truth, key_value_pairs) &&
           verifyServerSide(usz, value_sz, exp_seed);
  }

  bool SetUp_DDigest(size_t usz, size_t d, size_t value_sz, unsigned exp_seed,
                     double &completed_time) {
    SetUpRequest request;
    request.set_seed(exp_seed);
    request.set_d(d);
    request.set_usz(usz);
    request.set_next_algorithm(SetUpRequest_Method_DDigest);
    request.set_object_sz(value_sz);
    SetUpReply reply;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;
    // The actual RPC.
    Status status = stub_->ReconcileSetUp(&context, request, &reply);

    tsl::ordered_map<Key, Value> key_value_pairs;
    only_for_benchmark::GenerateKeyValuePairs<tsl::ordered_map<Key, Value>,
                                              Key>(key_value_pairs, usz,
                                                   value_sz, exp_seed);
    key_value_pairs.erase(key_value_pairs.cbegin(),
                          key_value_pairs.cbegin() + d);
    only_for_benchmark::SimpleTimer timer;
    completed_time = 0;
    timer.restart();
    auto succeed = Reconciliation_DDigest(key_value_pairs);
    completed_time = timer.elapsed();
    if (!succeed) return false;

    tsl::ordered_map<Key, Value> ground_truth;
    only_for_benchmark::GenerateKeyValuePairs<tsl::ordered_map<Key, Value>,
                                              Key>(ground_truth, usz, value_sz,
                                                   exp_seed);
    return only_for_benchmark::is_equal(ground_truth, key_value_pairs) &&
           verifyServerSide(usz, value_sz, exp_seed);
  }

  bool SetUp_Graphene(size_t usz, size_t d, size_t value_sz, unsigned exp_seed,
                      double &completed_time) {
    SetUpRequest request;
    request.set_seed(exp_seed);
    request.set_d(d);
    request.set_usz(usz);
    request.set_next_algorithm(SetUpRequest_Method_Graphene);
    request.set_object_sz(value_sz);
    SetUpReply reply;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;
    // The actual RPC.
    Status status = stub_->ReconcileSetUp(&context, request, &reply);

    tsl::ordered_map<Key, Value> key_value_pairs;
    only_for_benchmark::GenerateKeyValuePairs<tsl::ordered_map<Key, Value>,
                                              Key>(key_value_pairs, usz,
                                                   value_sz, exp_seed);
    only_for_benchmark::SimpleTimer timer;
    completed_time = 0;
    timer.restart();
    auto succeed = Reconciliation_Graphene(key_value_pairs);
    completed_time = timer.elapsed();
    if (!succeed) return false;

    tsl::ordered_map<Key, Value> ground_truth;
    only_for_benchmark::GenerateKeyValuePairs<tsl::ordered_map<Key, Value>,
                                              Key>(ground_truth, usz, value_sz,
                                                   exp_seed);
    return only_for_benchmark::is_equal(ground_truth, key_value_pairs) &&
           verifyServerSide(usz, value_sz, exp_seed);
  }

  bool SetUp_PBS(size_t usz, size_t d, size_t value_sz, unsigned exp_seed,
                 double &completed_time) {
    SetUpRequest request;
    request.set_seed(exp_seed);
    request.set_d(d);
    request.set_usz(usz);
    request.set_next_algorithm(SetUpRequest_Method_PBS);
    request.set_object_sz(value_sz);
    SetUpReply reply;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;
    // The actual RPC.
    Status status = stub_->ReconcileSetUp(&context, request, &reply);

    tsl::ordered_map<Key, Value> key_value_pairs;
    only_for_benchmark::GenerateKeyValuePairs<tsl::ordered_map<Key, Value>,
                                              Key>(key_value_pairs, usz,
                                                   value_sz, exp_seed);
    only_for_benchmark::SimpleTimer timer;
    completed_time = 0;
    timer.restart();
    auto succeed = Reconciliation_ParityBitmapSketch(key_value_pairs);
    completed_time = timer.elapsed();
    if (!succeed) return false;

    tsl::ordered_map<Key, Value> ground_truth;
    only_for_benchmark::GenerateKeyValuePairs<tsl::ordered_map<Key, Value>,
                                              Key>(ground_truth, usz, value_sz,
                                                   exp_seed);
    return only_for_benchmark::is_equal(ground_truth, key_value_pairs) &&
           verifyServerSide(usz, value_sz, exp_seed);
  }

  void Reconciliation_Experiments(size_t usz, size_t d, size_t value_sz,
                                  unsigned exp_seed, size_t repeats = 100,
                                  bool only_pbs = false) {
    std::string res_filename = fmt::format(
        "reconciliation_result_{}_{}_{}_{}.csv", usz, d, exp_seed, repeats);
    std::ofstream rfp(res_filename);
    if (!rfp.is_open()) {
      throw std::runtime_error(
          fmt::format("Failed to open {}!!", res_filename));
    }
    std::mt19937_64 gen(exp_seed);
    std::uniform_int_distribution<uint32_t> distribution;
    rfp << "#tid,algorithm,succeed,complete_time\n";
    fmt::print("{}\n", "#tid,algorithm,succeed,complete_time");
    double completed_time = 0;
    if (only_pbs) {
      for (size_t tid = 0; tid < repeats; ++tid) {
        auto seed = distribution(gen);
        auto succeed = SetUp_Graphene(usz, d, value_sz, seed, completed_time);
        rfp << fmt::format("{},{},{},{}\n", tid, "Graphene", (succeed ? 1 : 0),
                           completed_time);
        fmt::print("{},{},{},{}\n", tid, "Graphene", (succeed ? 1 : 0),
                   completed_time);
        completed_time = 0;
        succeed = SetUp_PBS(usz, d, value_sz, seed, completed_time);
        rfp << fmt::format("{},{},{},{}\n", tid, "PBS", (succeed ? 1 : 0),
                           completed_time);
        fmt::print("{},{},{},{}\n", tid, "PBS", (succeed ? 1 : 0),
                   completed_time);
      }
    } else {
      for (size_t tid = 0; tid < repeats; ++tid) {
        auto seed = distribution(gen);
        auto succeed = SetUp_DDigest(usz, d, value_sz, seed, completed_time);
        rfp << fmt::format("{},{},{},{}\n", tid, "DDigest", (succeed ? 1 : 0),
                           completed_time);
        fmt::print("{},{},{},{}\n", tid, "DDigest", (succeed ? 1 : 0),
                   completed_time);
        completed_time = 0;
        succeed = SetUp_PinSketch(usz, d, value_sz, seed, completed_time);
        rfp << fmt::format("{},{},{},{}\n", tid, "PinSketch", (succeed ? 1 : 0),
                           completed_time);
        fmt::print("{},{},{},{}\n", tid, "PinSketch", (succeed ? 1 : 0),
                   completed_time);
        completed_time = 0;
        succeed = SetUp_PBS(usz, d, value_sz, seed, completed_time);
        rfp << fmt::format("{},{},{},{}\n", tid, "PBS", (succeed ? 1 : 0),
                           completed_time);
        fmt::print("{},{},{},{}\n", tid, "PBS", (succeed ? 1 : 0),
                   completed_time);
      }
    }
  }

  // template<typename Key, typename Value>
  bool Reconciliation_DDigest(tsl::ordered_map<Key, Value> &key_value_pairs,
                              ssize_t d = -1) {
    size_t scaled_d = d;
    if (d == -1) {
      auto est = EstimationKeyValuePairs(key_value_pairs.cbegin(),
                                         key_value_pairs.cend());
      scaled_d = ESTIMATE_SM99(est);
    }

    const int VAL_SIZE = 1;  // bytes
    const std::vector<uint8_t> VAL{0};
    float HEDGE =
        2.0;  // use suggested value provided in what's difference paper
    IBLT my_iblt(scaled_d, VAL_SIZE, HEDGE, (scaled_d > 200 ? 3 : 4));

    for (const auto &kv : key_value_pairs) {
      my_iblt.insert(kv.first, VAL);
    }
    // serialization
    DDigestRequest request;
    auto table = my_iblt.data();
    for (const auto &each_cell : table) {
      auto cell = request.mutable_cells()->Add();
      cell->set_count(each_cell.count);
      cell->set_keysum(each_cell.keySum);
      cell->set_keycheck(each_cell.keyCheck);
    }

    DDigestReply reply;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;
    // The actual RPC.
    Status status = stub_->ReconcileDDigest(&context, request, &reply);
    // Act upon its status.
    if (!status.ok()) {
      std::cerr << (std::to_string(status.error_code()) + ": " +
                    status.error_message())
                << std::endl;
      return false;
    }
    if (!reply.succeed()) return false;
    for (const auto &kv : reply.pushed_key_values()) {
      if (key_value_pairs.contains(kv.key())) return false;
      key_value_pairs.insert({kv.key(), kv.value()});
    }
    if (!reply.missing_keys().empty())
      return Push(reply.missing_keys().cbegin(), reply.missing_keys().cend(),
                  key_value_pairs);
    return true;
  }

  // template<typename Key, typename Value>
  bool Reconciliation_Graphene(tsl::ordered_map<Key, Value> &key_value_pairs) {
    GrapheneRequest request;
    request.set_m(key_value_pairs.size());

    GrapheneReply reply;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;
    // The actual RPC.
    Status status = stub_->ReconcileGraphene(&context, request, &reply);
    // Act upon its status.
    if (!status.ok()) {
      std::cerr << (std::to_string(status.error_code()) + ": " +
                    status.error_message())
                << std::endl;
      return false;
    }

    const std::vector<uint8_t> DUMMY_VAL{0};
    const int VAL_SIZE = 1;  // bytes
    auto a = reply.a();
    std::vector<int> Z_compl;
    auto iblt_receiver_first = IBLT(a, VAL_SIZE);
    bool no_bf = reply.bf().empty();
    std::unique_ptr<bloom_filter> bloom_sender_ptr;  // empty ptr
    if (!no_bf) {
      bloom_parameters params;
      params.projected_element_count = reply.n();
      params.false_positive_probability = reply.fpr();
      params.compute_optimal_parameters();
      bloom_sender_ptr = std::make_unique<bloom_filter>(params);
      bloom_sender_ptr->set(reply.bf().cbegin(), reply.bf().cend());
      for (const auto &kv : key_value_pairs) {
        if (bloom_sender_ptr->contains(kv.first)) {
          iblt_receiver_first.insert(kv.first, DUMMY_VAL);
        } else
          Z_compl.push_back(kv.first);
      }
    } else {
      for (const auto &kv : key_value_pairs) {
        iblt_receiver_first.insert(kv.first, DUMMY_VAL);
      }
    }

    auto iblt_sender_first = IBLT(a, VAL_SIZE);

    using cell_iterator_t = decltype(reply.ibf().cbegin());
    iblt_sender_first.set(
        reply.ibf().cbegin(), reply.ibf().cend(),
        [](cell_iterator_t it) { return it->count(); },
        [](cell_iterator_t it) { return it->keysum(); },
        [](cell_iterator_t it) { return it->keycheck(); });
    std::set<std::pair<uint64_t, std::vector<uint8_t>>> pos, neg;
    // Eppstein subtraction
    auto ibltT = iblt_receiver_first - iblt_sender_first;
    bool correct = ibltT.listEntries(pos, neg);
    if (!correct) return false;

    for (const auto &key : pos) {
      Z_compl.push_back(key.first);
    }

    Push(Z_compl.begin(), Z_compl.end(), key_value_pairs);

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
      return Push(reply.missing_keys().cbegin(), reply.missing_keys().cend(),
                  key_value_pairs);
    return true;
  }

  // template<typename Key, typename Value>
  bool Reconciliation_ParityBitmapSketch(
      tsl::ordered_map<Key, Value> &key_value_pairs, ssize_t d = -1) {
    size_t scaled_d = d;
    if (d == -1) {
      auto est = EstimationKeyValuePairs(key_value_pairs.cbegin(),
                                         key_value_pairs.cend());
      scaled_d = ESTIMATE_SM99(est);
    }

    auto _pbs = std::make_unique<libpbs::ParityBitmapSketch>(scaled_d);
    for (const auto &kv : key_value_pairs) _pbs->add(kv.first);

    bool completed = false, syn_completed = false;

    std::vector<uint64_t> res;

    do {
      std::vector<uint64_t> xors, checksums, missing;

      if (completed) {
        // set reconciliation completed
        std::vector<Key> pushed_keys;
        for (const auto &k : res) {
          if (!key_value_pairs.contains(k))
            missing.push_back(k);
          else
            pushed_keys.push_back(k);
        }
        if (!missing.empty() && !pushed_keys.empty())
          PushAndPull(pushed_keys.cbegin(), pushed_keys.cend(),
                      missing.cbegin(), missing.cend(), key_value_pairs);
        else if (!missing.empty())
          Pull(missing.cbegin(), missing.cend(), key_value_pairs);
        else if (!pushed_keys.empty())
          Push(pushed_keys.cbegin(), pushed_keys.cend(), key_value_pairs);
        syn_completed = true;
        break;
      }

      if (_pbs->rounds() >= PBS_MAX_ROUNDS) {
        break;
      }

      auto [enc, hint] = _pbs->encode();

      PbsRequest request;
      request.mutable_encoding_msg()->resize(enc->serializedSize(), 0);
      enc->write((uint8_t *)&(*request.mutable_encoding_msg())[0]);

      if (hint != nullptr) {
        request.mutable_encoding_hint()->resize(hint->serializedSize(), 0);
        hint->write((uint8_t *)&(*request.mutable_encoding_hint())[0]);
      }

      if (!res.empty()) {
        for (const auto &k : res) {
          if (key_value_pairs.contains(k)) {
            auto kv = request.mutable_pushed_key_values()->Add();
            kv->set_key(k);
            kv->set_value(key_value_pairs[k]);
          } else {
            *(request.mutable_missing_keys()->Add()) = k;
          }
        }
      }

      PbsReply reply;
      ClientContext context;

      // The actual RPC.
      Status status =
          stub_->ReconcileParityBitmapSketch(&context, request, &reply);

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

      libpbs::PbsDecodingMessage decoding_message(
          _pbs->bchParameterM(), _pbs->bchParameterT(), _pbs->numberOfGroups());

      decoding_message.parse((const uint8_t *)reply.decoding_msg().c_str(),
                             reply.decoding_msg().size());

      xors.insert(xors.end(), reply.xors().cbegin(), reply.xors().cend());
      checksums.insert(checksums.end(), reply.checksum().cbegin(),
                       reply.checksum().cend());
      completed = _pbs->decodeCheck(decoding_message, xors, checksums);
      res = _pbs->differencesLastRound();

      fmt::print("Round #{}: {:>6} of {:>6} decoded\n", _pbs->rounds(),
                 res.size(), d);

    } while (true);

    return syn_completed;
  }

  template <typename Iterator>
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
  template <typename Iterator>
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

#endif  // RECONCILIATION_CLIENT_H_
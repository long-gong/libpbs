/**
 * @file pbs.h
 * @author Long Gong <long.github@gmail.com>
 * @brief Parity Bitmap Schetch for reconciliation
 * @version 0.1
 * @date 2020-07-15
 *
 * @copyright Copyright (c) 2020
 */
#ifndef PARITY_BITMAP_SKETCH_H_
#define PARITY_BITMAP_SKETCH_H_

#include <vector>
#include <map>
#include <memory>
#include <utility>
#include <xxh3.h>

#include <minisketch.h>

#include "pbs_params.h"
#include "pbs_encoding_message.h"
#include "pbs_decoding_message.h"
#include "pbs_decoding_hint_message.h"

/**
 * @brief The macro for hash functions
 */
#define MY_HASH_FN(key, seed) XXH3_64bits_withSeed(&key, sizeof(key), seed)

namespace libpbs {
namespace {
/**
 * @brief The constants
 */
constexpr unsigned DEFAULT_MAX_ROUNDS = 3;
constexpr float DEFAULT_AVG_DIFFS_PER_GROUP = 5;
constexpr unsigned DEFAULT_NUM_GROUPS_WHEN_BCH_FAIL = 3;
constexpr double DEFAULT_TARGET_SUCCESS_PROB = 0.99;

const uint64_t DEFAULT_SEED_G = 0x6d496e536b65LU;
const uint64_t SEED_OFFSET = 142857;

/**
 * @brief PBS states
 *
 */
enum class PbsState {
  INIT,
  ADD,
  ENCODE,
  DECODE,
  DECODE_CHECK,
  COMPLETED
};

enum class PbsRole {
  Alice,
  Bob,
  Undetermined
};
} // namespace


using key_t = uint64_t;
using bitmap_t = std::vector<uint8_t>;

/**
 * @brief ParityBitmapSketch class
 *
 */
class ParityBitmapSketch {
 public:
  /**
   * @brief Constructor
   *
   *
   * @param num_diffs                     number of distinct elements (an accurate estimate or exact)
   * @param avg_diffs_per_group           average number of distinct elements per group
   * @param target_success_prob           target success probability (within `max_rounds`)
   * @param max_rounds                    maximum number of rounds (for achieving the target success probability)
   * @param num_groups_when_bch_fail      number of sub-groups to be further split when BCH decoding failed
   * @param seed                          random seed
   */
  ParityBitmapSketch(uint32_t num_diffs,
                     float avg_diffs_per_group = DEFAULT_AVG_DIFFS_PER_GROUP,
                     double target_success_prob = DEFAULT_TARGET_SUCCESS_PROB,
                     unsigned max_rounds = DEFAULT_MAX_ROUNDS,
                     unsigned num_groups_when_bch_fail = DEFAULT_NUM_GROUPS_WHEN_BCH_FAIL,
                     uint64_t seed = DEFAULT_SEED_G) :
      avg_diffs_per_group_(avg_diffs_per_group),
      target_success_prob_(target_success_prob),
      max_rounds_(max_rounds),
      num_groups_when_bch_fail_(num_groups_when_bch_fail),
      group_partition_seed_(seed),
      parity_encoding_seed_(seed + SEED_OFFSET),
      num_diffs_(num_diffs),
      num_groups_(static_cast<std::size_t>(std::ceil((float) num_diffs / avg_diffs_per_group))),
      num_groups_remaining_(num_groups_),
      round_count_(0),
      state_(PbsState::INIT),
      role_(PbsRole::Undetermined),
      groups_(num_groups_),
      to_original_group_id_(num_groups_),
      pbs_encoding_(nullptr),
      pbs_decoding_(nullptr),
      checksums_(num_groups_, 0) {
    calcBchParams_();
    xors_.resize(num_groups_ * bch_n_, 0);
    for (size_t gid = 0; gid < num_groups_; ++gid) to_original_group_id_[gid] = gid;
  }

  /**
   * @brief  Add a single element
   *
   * Please add all elements before calling encoding.
   *
   * @param element   element to be added
   */
  void add(uint64_t element) {
    auto gid = getGroupId_(element);
    groups_[gid].push_back(element);
  }

  /**
   * @brief Batch adding elements
   *
   * @tparam Iterator
   * @param first, last    	the range of elements to add
   */
  template<typename Iterator>
  void add(Iterator first, Iterator last) {
    for (auto it = first; it != last; ++it) add(static_cast<uint64_t>(*it));
  }

  /**
   * @brief Encoding
   *
   * @return    a pair of PBS encoding message and PBS decoding hint message (could be null)
   */
  std::pair<std::shared_ptr<PbsEncodingMessage>, std::shared_ptr<PbsDecodingHintMessage>> encode() {
    std::shared_ptr<PbsDecodingHintMessage> hint(nullptr);
    pbs_encoding_ = std::make_shared<PbsEncodingMessage>(bch_m_, bch_t_, num_groups_remaining_);
    for (size_t gid = 0; gid < num_groups_remaining_; ++gid) doEncode_(gid, pbs_encoding_->getSketch(gid));
    if (!groups_exp_I_or_II_.empty()) {
      hint = std::make_shared<PbsDecodingHintMessage>(groups_exp_I_or_II_.size());
      for (size_t gid: groups_exp_I_or_II_)
        hint->addGroupId(gid);
      groups_exp_I_or_II_.clear();
    }
    return {pbs_encoding_, hint};
  }

  /**
   * @brief Encoding with hint (needed when handling exceptions)
   *
   * This function should be used when you receive a PBS Decoding hint message
   *
   * @tparam Iterator
   * @param first, last        the range of groups to doEncode_
   * @return                   a shared pointer to a PBS encoding message
   */
  template<typename Iterator>
  std::shared_ptr<PbsEncodingMessage> encodeWithHint(Iterator first, Iterator last) {
    if (role_ != PbsRole::Bob) throw std::logic_error("Only Bob do doEncode_ with hint");
    size_t num_groups_I_or_II = std::distance(first, last);
    auto num_groups = num_groups_I_or_II + groups_bch_failed_.size() * num_groups_when_bch_fail_;
    pbs_encoding_ = std::make_shared<PbsEncodingMessage>(bch_m_, bch_t_, num_groups);
    size_t i = 0;
//    if (groups_to_be_handled_.empty()) {
    xors_.resize(xors_.size() + num_groups_I_or_II * bch_n_, 0u);
    checksums_.resize(checksums_.size() + num_groups_I_or_II, 0u);

    for (auto it = first; it != last; ++it) {
      auto old_gid = *it;
      groups_.push_back(std::move(groups_[old_gid]));
      to_original_group_id_.push_back(to_original_group_id_[old_gid]);
      doEncode_(groups_.size() - 1, pbs_encoding_->getSketch(i++));
    }

    // remove all successful groups (and its associated all info)
    update_();
//    } else {
//      for (auto it = first; it != last; ++it)
//        doEncode_(groups_to_be_handled_[*it],
//                pbs_encoding_->getSketch(i++),
//                true /* need initialization */);
//    }
    //
//    groups_to_be_handled_.clear();
//    groups_to_be_handled_.insert(groups_to_be_handled_.end(), first, last);
    return pbs_encoding_;
  }

  /**
   * @brief  Decoding
   *
   * @param other                PBS encoding message received from the other host
   * @param xors                 XORs to be sent to the other host
   * @param checksums            checksums to be sent to the other host
   * @return                     a shared pointer to PBS decoding message
   */
  std::shared_ptr<PbsDecodingMessage> decode(const PbsEncodingMessage &other,
                                             std::vector<key_t> &xors,
                                             std::vector<key_t> &checksums) {
    if (role_ == PbsRole::Alice) std::logic_error("Alice can not do decode");
    role_ = PbsRole::Bob;
    pbs_decoding_ = std::make_shared<PbsDecodingMessage>(bch_m_, bch_t_, num_groups_remaining_);
//                                                         (groups_to_be_handled_.empty() ? num_groups_
//                                                                                        : groups_to_be_handled_.size()));
    pbs_decoding_->setWith(pbs_encoding_->getSketches().begin(),
                           pbs_encoding_->getSketches().end(),
                           other.getSketches().cbegin());
    size_t offset = 0, gid = 0;
    for (const auto &p: pbs_decoding_->decoded_num_differences) {
//      auto gid = (groups_to_be_handled_.empty() ? i : groups_to_be_handled_[i]);
      if (p >= 0) {
        for (size_t k = 0; k < p; ++k) {
          auto bid = pbs_decoding_->decoded_differences[offset + k];
          xors.push_back(0);
          xors.back() ^= xors_[gid * bch_n_ + bid];
        }
        offset += p;
        checksums.push_back(checksums_[gid]);
      } else {
        // BCH decoding failed
        threeWaySplit_(gid);
      }
      gid++;
    }

    // update number rounds
    ++round_count_;
    return pbs_decoding_;
  }

  /**
   * @brief
   *
   * @param msg
   * @param xors
   * @param checksums
   * @return
   */
  bool decodeCheck(
      const PbsDecodingMessage &msg,
      const std::vector<key_t> &xors,
      const std::vector<key_t> &checksums
  ) {
    if (role_ == PbsRole::Bob) throw std::logic_error("Bob can not do decode check");
    role_ = PbsRole::Alice;
    recovered_.push_back(std::vector<key_t>());
//    auto no_exception = groups_to_be_handled_.empty() ?
//                        decodeCheckAll_(msg, xors, checksums) : decodeCheckPartial_(msg, xors, checksums);
    size_t offset = 0, cid = 0;
    groups_bch_failed_.clear();
    // handle BCH failure first (to make sure groups of Alice and Bob will
    // have the same order)
    for (size_t gid = 0; gid < msg.num_groups; ++gid) {
      auto p = msg.decoded_num_differences[gid];
      if (p < 0) {
        groups_bch_failed_.push_back(gid);
        threeWaySplit_(gid);
      }
    }

    groups_exp_I_or_II_.clear();
    for (size_t gid = 0; gid < msg.num_groups; ++gid) {
      auto p = msg.decoded_num_differences[gid];
      // Note that checksum is available ONLY when p >= 0
      if (p >= 0) {
        doDecodeCheck_(gid, p, &(msg.decoded_differences[offset]), &xors[offset], checksums[cid++]);
        offset += p;
      }
//      if (p >= 0) cid++;
    }

    update_();
    // update number rounds
    ++round_count_;
    return (num_groups_remaining_ == 0);
  }

  const std::vector<key_t> &differencesLastRound() const {
    return recovered_.back();
  }

  const std::vector<std::vector<key_t>> differencesAll() const {
    return recovered_;
  }

  const std::string name() const {
    return "ParityBitMapSketch";
  }

 private:
  // average number of differences (the elements that only one of the sets A, B has) in each group
  float avg_diffs_per_group_;
  // target success probability within `max_rounds_`
  double target_success_prob_;
  // target upper bound for the number of rounds to achieve the target success probability
  unsigned max_rounds_;
  // how many number of sub-groups to use when BCH docoding failed in a group
  unsigned num_groups_when_bch_fail_;

  // seed for group partition
  uint64_t group_partition_seed_;
  // seed for parity bitmap encoding
  uint64_t parity_encoding_seed_;

  /* BCH related parameters */
  size_t bch_m_;
  // BCH block length (bch_n_ = 2^bch_m_ - 1)
  size_t bch_n_;
  // BCH error-correcting capacity
  size_t bch_t_;

  // good estimate for the cardinality of the set difference
  // or the exact cardinality of the set difference
  size_t num_diffs_;
  // number of groups in the first round
  size_t num_groups_;
  size_t num_groups_remaining_;

  // current number of rounds
  uint32_t round_count_;
  // current state (to be used)
  PbsState state_;
  PbsRole role_;

  // element groups
  std::vector<std::vector<key_t >> groups_;
  std::vector<size_t> to_original_group_id_;
  // std::vector<minisketch*> sketches_;
  /* PBS messages */
  std::shared_ptr<PbsEncodingMessage> pbs_encoding_;
  std::shared_ptr<PbsDecodingMessage> pbs_decoding_;
  // XOR of all elements in each bin
  std::vector<key_t> xors_;
  // checksum (XOR of all elements) for a group
  std::vector<key_t> checksums_;
  // map for group id (ONLY used when BCH decoding failure happens)
//  std::map<size_t, size_t> group_id_map_;
  // groups where exceptions happened
  std::vector<size_t> groups_exp_I_or_II_;
  std::vector<size_t> groups_bch_failed_;

  // recovered elements
  std::vector<std::vector<key_t>> recovered_;

  /**
   * @brief Calculate the near-optimal BCH parameters for PBS
   *
   */
  void calcBchParams_() {
    pbsutils::BestBchParam bch_param;
    pbsutils::PbsParam::bestBchParam(num_diffs_, avg_diffs_per_group_,
                                     max_rounds_, num_groups_when_bch_fail_,
                                     target_success_prob_, bch_param);
    // TODO: release caches
    bch_m_ = bch_param.n;
    bch_n_ = (1u << bch_param.n) - 1;
    bch_t_ = bch_param.t;
  }

  /**
   * @brief Get which group `element` is partitioned to
   *
   * @param element       element to be checked
   * @return              group id
   */
  inline uint64_t getGroupId_(uint64_t element) const {
    return MY_HASH_FN(element, group_partition_seed_) % num_groups_;
  }

  /**
   * @brief Get which bin should `element` be located
   *
   *
   * @param element   element to be checked
   * @return          bin id
   */
  inline uint64_t getBinId_(uint64_t element) const {
    /// Note that here we do not use the bin with id 0
    /// since minisketch now does not support 0
    /// Note also that the seed needs to be changed for each round
    return MY_HASH_FN(element, parity_encoding_seed_ + round_count_) % (bch_n_ - 1) + 1;
  }

  /**
   * @brief Encode for a single group
   *
   * @param gid                  group id
   * @param sketch               the minisketch for this group
   * @param init_before_set      whether need initialize before set the values
   */
  void doEncode_(size_t gid, minisketch *sketch) {
    bitmap_t bitmap(bch_n_, 0u);
    size_t xor_start = gid * bch_n_;
//    if (init_before_set) {
//      checksums_[gid] = 0;
//      std::fill(xors_.begin() + xor_start, xors_.begin() + xor_start + bch_n_, 0);
//    }

    for (const auto elm : groups_[gid]) {
      size_t loc = getBinId_(elm);
      bitmap[loc] ^= 1u;
      xors_[xor_start + loc] ^= elm;
      checksums_[gid] ^= elm;
    }

    for (uint64_t k = 0; k < bch_n_; ++k) {
      if (bitmap[k]) minisketch_add_uint64(sketch, k);
    }
  }

  /**
   * @brief BCH decoding failure handler
   *
   * @param gid       group id (the BCH decoding failure happens in this group)
   */
  inline void threeWaySplit_(size_t gid) {
    // BCH decoding failed
    size_t old_size = groups_.size();
    groups_.resize(old_size + num_groups_when_bch_fail_);
    for (size_t k = 0; k < groups_[gid].size(); ++k) {
      size_t index = MY_HASH_FN(groups_[gid][k], group_partition_seed_ + round_count_) %
          num_groups_when_bch_fail_;
      groups_[old_size + index].push_back(groups_[gid][k]);
    }

    xors_.resize(xors_.size() + num_groups_when_bch_fail_ * bch_n_, 0);
    checksums_.resize(checksums_.size() + num_groups_when_bch_fail_, 0);
    to_original_group_id_.resize(to_original_group_id_.size() + num_groups_when_bch_fail_, to_original_group_id_[gid]);
//    for (size_t i = old_size; i < old_size + num_groups_when_bch_fail_; ++i) {
////      groups_to_be_handled_.push_back(i);
////      group_id_map_[i] = (gid < num_groups_ ? gid : group_id_map_[gid]);
//      to_original_group_id_.push_back(to_original_group_id_)
//    }
  }
  /**
   * @brief Check whether there are exceptions in a group
   *
   * @param gid                  group id
   * @param p                    how many "bit errors" were pinpointed by BCH
   * @param bin_id_start         pointer to "bit error positions"
   * @param a_xor                pointer to XORs
   * @param checksum             checksum
   */
  void doDecodeCheck_(
      std::size_t gid,
      ssize_t p,
      const uint64_t *bin_id_start,
      const key_t *a_xor,
      key_t checksum
  ) {

//    if (p == -1) {
////      threeWaySplit_(gid);
//      groups_bch_failed_.push_back(gid);
//      return;
//    }

    std::vector<key_t> recovered;
    key_t b_checksum = checksums_[gid];
    size_t b_xor_start = bch_n_ * gid;
    for (ssize_t i = 0; i < p; ++i) {
      size_t bid = bin_id_start[i];
      auto elm = a_xor[i] ^xors_[b_xor_start + bid];
      size_t expected_gid = to_original_group_id_[gid];
      size_t obtained_gid = getGroupId_(elm);
      size_t obtained_bid = getBinId_(elm);
      if (obtained_bid == bid && obtained_gid == expected_gid) {
        // with very high probability, elm here is a correct distinct element
        // Note that even if it turned out to be a fake, our algorithm can automatically
        // fixes it in later rounds
        recovered.push_back(elm);
        b_checksum ^= elm;
      }
    }
    // insert into results
    recovered_[round_count_].insert(recovered_[round_count_].end(), recovered.begin(), recovered.end());
    // checksum verification
    if (checksum != b_checksum) {
      /// type (I) and/or (II) exception happened
      // Note that, here we actually should change this corresponding group
      // to the set difference between it and the recovered set.
      // However, we simply appended all recovered elements into this group,
      // because all our encoding operators are XOR-like, which means the common elements
      // will be automatically cancelled out.
      groups_[gid].insert(groups_[gid].end(), recovered.begin(), recovered.end());
      // notice this encoder this group needs further handling
      // groups_to_be_handled_.push_back(gid);
      groups_.push_back(std::move(groups_[gid]));
//      groups_to_be_handled_.push_back(groups_.size() - num_groups_remaining_);
      to_original_group_id_.push_back(to_original_group_id_[gid]);

      xors_.resize(xors_.size() + bch_n_, 0u);
      checksums_.push_back(0u);
      groups_exp_I_or_II_.push_back(gid);
    }
  }

  inline void update_() {
    for (auto gid: groups_bch_failed_) threeWaySplit_(gid);
    groups_.erase(groups_.begin(), groups_.begin() + num_groups_remaining_);
    xors_.erase(xors_.begin(), xors_.begin() + bch_n_ * num_groups_remaining_);
    checksums_.erase(checksums_.begin(), checksums_.begin() + num_groups_remaining_);
    to_original_group_id_.erase(to_original_group_id_.begin(), to_original_group_id_.begin() + num_groups_remaining_);
    num_groups_remaining_ = groups_.size();
  }

  /***
   * @brief Check whether there are exceptions in certain groups
   *
   * For more details, please refer to our paper.
   *
   * @param msg             PBS decoding message instance
   * @param xors            XORs associated with the PBS decoding message
   * @param checksums       checksums associated with the PBS decoding message
   * @return                whether there is no exceptions (true -- no exception)
   */
//  bool decodeCheckPartial_(
//      const PbsDecodingMessage &msg,
//      const std::vector<key_t> &xors,
//      const std::vector<key_t> &checksums
//  ) {
////    std::vector<size_t> groups = std::move(groups_to_be_handled_);
////    assert (msg.num_groups == groups.size());
//    size_t offset = 0, cid = 0;
//
//    for (size_t gid = 0; gid < msg.num_groups; ++gid) {
//      auto p = msg.decoded_num_differences[gid];
//      // Note that checksum is available ONLY when p >= 0
//      doDecodeCheck_(gid, p, &(msg.decoded_differences[offset]), &xors[offset], checksums[cid]);
//      if (p >= 0) cid++;
//      offset += (p < 0 ? 0 : p);
//    }
//
//    update_();
//
////    return groups_to_be_handled_.empty();
//    return (num_groups_remaining_ == 0);
//  }

  /***
   * @brief Check whether there are exceptions in all groups
   *
   * For more details, please refer to our paper.
   *
   * @param msg             PBS decoding message instance
   * @param xors            XORs associated with the PBS decoding message
   * @param checksums       checksums associated with the PBS decoding message
   * @return                whether there is no exceptions (true -- no exception)
   */
//  bool decodeCheckAll_(
//      const PbsDecodingMessage &msg,
//      const std::vector<key_t> &xors,
//      const std::vector<key_t> &checksums
//  ) {
//    assert (msg.num_groups == num_groups_);
//    size_t offset = 0, cid = 0;
//    for (size_t gid = 0; gid < num_groups_; ++gid) {
//      auto p = msg.decoded_num_differences[gid];
//      // Note that checksum is available ONLY when p >= 0
//      doDecodeCheck_(gid, p, &(msg.decoded_differences[offset]), &xors[offset], checksums[cid]);
//      if (p >= 0) cid++;
//      offset += (p < 0 ? 0 : p);
//    }
//
//    update_();
////    return groups_to_be_handled_.empty();
//    return num_groups_remaining_ == 0;
//  }
};
} // end namespace libpbs
#endif //PARITY_BITMAP_SKETCH_H_

#ifndef PBS_H_
#define PBS_H_

/// It seems some headers were conflicted with boost::dynamic_bitset (Please do
/// not move its position)
#include <boost/dynamic_bitset.hpp>
#include <minisketch.h>
#include <xxh3.h>

#include <cmath>
#include <random>
#include <string>
#include <vector>

#include "bit_utils.h"
#include "pbs_params.h"

namespace {
constexpr unsigned DEFAULT_MAX_ROUNDS = 3;
constexpr float DEFAULT_AVG_DIFFS_PER_GROUP = 5;
constexpr unsigned DEFAULT_NUM_GROUPS_WHEN_BCH_FAIL = 3;
constexpr double DEFAULT_TARGET_SUCCESS_PROB = 0.99;

const unsigned DEFAULT_SEED_SEED = 142857;
const unsigned SEED_SALT = 1;

}  // namespace

using BitMap = boost::dynamic_bitset<>;
template <typename KeyType>
class ParityBitmapSketch {
 public:
  ParityBitmapSketch(
      size_t num_diffs,
      double target_success_prob = DEFAULT_TARGET_SUCCESS_PROB,
      unsigned max_rounds = DEFAULT_MAX_ROUNDS,
      float avg_diffs_per_group = DEFAULT_AVG_DIFFS_PER_GROUP,
      unsigned num_groups_when_bch_fail = DEFAULT_NUM_GROUPS_WHEN_BCH_FAIL)
      : num_diffs_(num_diffs),
        max_rounds_(max_rounds),
        avg_diffs_per_group_(avg_diffs_per_group),
        num_groups_when_bch_fail_(num_groups_when_bch_fail),
        current_round_(0),
        remaining_num_groups_(
            std::ceil((double)num_diffs / avg_diffs_per_group_)),
        initial_num_groups_(remaining_num_groups_),
        target_success_prob_(target_success_prob),
        bch_m_(0),
        bch_length_(0),
        bch_capacity_(0),
        seed_pool_{},
        groups_(remaining_num_groups_),
        xor_(remaining_num_groups_),
        bitmaps_(remaining_num_groups_),
        group_ids_(remaining_num_groups_) {
    calc_bch_params();
    for (auto &bins : xor_) bins.resize(bch_length_, 0);
    for (auto &bitmap : bitmaps_) bitmap.resize(bch_length_);
  }

  std::string name() const { return "PBS"; }
  unsigned current_round() const { return current_round_; }
  unsigned avg_diffs_per_group() const { return avg_diffs_per_group_; }
  unsigned num_groups_when_bch_fail() const {
    return num_groups_when_bch_fail_;
  }
  unsigned remaining_num_groups() const { return remaining_num_groups_; }
  size_t num_diffs() const { return num_diffs_; }

  template <typename Iterator>
  std::vector<std::string> encode(Iterator first, Iterator last) {
    hash_partition(first, last);
    return serilization();
  }

  std::vector<std::string> serilization() {
    std::vector<std::string> buffers;
    buffers.reserve(remaining_num_groups_);
    for (size_t g = 0; g < remaining_num_groups_; ++g)
      buffers.push_back(encode_single_group(g));
    return buffers;
  }

  template <typename Iterator>
  bool decode_from_xors(unsigned char *header, Iterator xor_first,
                        Iterator xor_last, std::vector<KeyType> &reconciled) {
    const size_t decode_header_len = std::ceil(std::log2(bch_capacity_ + 2));
    const uint32_t bch_failed_flag = (1u << decode_header_len) - 1u;
    std::vector<uint32_t> temp, temp2;
    BitReader reader(header);
    size_t i = 0, num_decoded = 0, bin_loc_offset = 0, xor_offset = 0;

    std::vector<size_t> bch_failed_groups;
    for (; i < remaining_num_groups_; ++i) {
      temp.push_back(reader.Read<uint32_t>(decode_header_len));
      if (temp.back() != bch_failed_flag) {
        num_decoded += temp.back();
      } else {
        bch_failed_groups.push_back(i);
      }
    }
    for (size_t j = 0; j < num_decoded; ++j)
      temp2.push_back(reader.Read<uint32_t>(bch_capacity_));

    {
      i = 0;
      for (; i < remaining_num_groups_; ++i) {
        if (temp[i] != bch_failed_flag) {
          ssize_t p = temp[i];
          vector<uint> ind(bch_length_, 0);
          for (int k = 0; k < p; ++k) {
            ind[temp2[k + bin_loc_offset]] = k + 1;
          }
          for (size_t bi = 1; bi < bch_length_; ++bi) {
            xor_first[xor_offset + ind[bi]] ^= xor_[i][bi];
          }
          bin_loc_offset += p;

          if (xor_first[xor_offset] != 0) {
            size_t old_size = groups_.size();
            groups_.resize(old_size + 1);
            group_ids_.resize(old_size + 1);
            for (size_t k = 0; k < groups_[i].size(); ++k) {
              size_t loc = get_loc(i, k);
              if (!ind[loc]) {
                groups_[old_size].push_back(groups_[i][k]);
              }
            }
            group_ids_[old_size] = group_ids_[i];
          }

          for (int k = 1; k <= p; ++k) {
            auto possible_diff = xor_first[xor_offset + k];
            if (possible_diff) {
              size_t group_id = get_group_id(possible_diff);
              size_t bin_index = get_bin_index(possible_diff);
              if (bin_index == temp2[k - 1 + bin_loc_offset] && group_id ==
                      group_ids_[i]) {
                reconciled.push_back(possible_diff);
                size_t old_size = groups_.size();
                groups_.resize(old_size + 1);
                group_ids_.resize(old_size + 1);
                for (size_t q = 0; q < groups_[i].size(); ++q) {
                  size_t loc = get_loc(i, q);
                  if (loc == temp2[k - 1 + bin_loc_offset])
                    groups_[old_size].push_back(groups_[i][q]);
                }
                group_ids_[old_size] = group_ids_[i];
              }
            }
          }

          xor_offset += p + 1;
        }
      }
    }

    for (auto j : bch_failed_groups) {
      size_t old_size = groups_.size();
      groups_.resize(old_size + num_groups_when_bch_fail_);

      for (size_t k = 0; k < groups_[j].size(); ++k) {
        size_t index = hash(groups_[i][k], group_partition_seed()) %
                       num_groups_when_bch_fail_;
        groups_[old_size + index].push_back(groups_[j][k]);
      }
    }

    groups_.erase(groups_.begin(), groups_.begin() + remaining_num_groups_);
    group_ids_.erase(group_ids_.begin(),
                     group_ids_.begin() + remaining_num_groups_);
    remaining_num_groups_ = groups_.size();

    seed_pool_.next();
    ++current_round_;

    if (remaining_num_groups_ > 0) {
      xor_.resize(remaining_num_groups_);
      for (auto &group_xors : xor_) group_xors.reset();
    } else {
      xor_.clear();
      return true;
    }

    return false;
  }

  template <typename Iterator, typename Iterator2>
  void decode(Iterator other_first, Iterator other_last, Iterator2 first,
              Iterator2 last, unsigned char *header,
              std::vector<KeyType> &xors) {
    const size_t decode_header_len = std::ceil(std::log2(bch_capacity_ + 2));
    const uint32_t bch_failed_flag = (1u << decode_header_len) - 1u;

    xors.clear();

    hash_partition(first, last);
    std::vector<uint32_t> temp, temp2;
    auto it = other_first;
    for (size_t g = 0, bi = 0; g < remaining_num_groups_ && it != other_last;
         ++g, bi += decode_header_len) {
      std::vector<uint64_t> identified_bins;
      auto p =
          decode_single_group((unsigned char *)&((*it)[0]), g, identified_bins);
      if (p < 0) {
        temp.push_back(bch_failed_flag);
      } else {
        temp.push_back((uint32_t)p);
        for (ssize_t j = 0; j < p; ++j) {
          temp2.push_back((uint32_t)identified_bins[j]);
        }
        std::vector<unsigned> ind(bch_length_, 0);
        size_t offset = xors.size();
        xors.resize(offset + p + 1, 0);
        for (int k = 0; k < p; ++k) {
          ind[identified_bins[k]] = k + 1;
        }
        for (size_t xi = 1; xi < bch_length_; ++xi) {
          xors[offset + ind[xi]] ^= xor_[g][xi];
        }
      }
    }

    size_t header_len = (decode_header_len * remaining_num_groups_ +
                         temp2.size() * bch_m_ + 7) /
                        8;
    header = (unsigned char *)malloc(header_len);
    memset(header, 0, header_len);
    BitWriter writer(header);
    for (const auto &i : temp) writer.Write(i, decode_header_len);
    for (const auto &i : temp2) writer.Write(i, bch_m_);
    writer.Flush();
  }

 private:
  struct SeedPool {
    SeedPool() : g(DEFAULT_SEED_SEED), distrib(1), current_seed(0) { next(); }
    std::mt19937_64 g;
    std::uniform_int_distribution<uint32_t> distrib;
    uint32_t current_seed;
    uint32_t previous_seed;
    uint32_t next() {
      previous_seed = current_seed;
      current_seed = distrib(g);
      return current_seed;
    }
    uint32_t current() { return current_seed; }
    uint32_t previous() { return previous_seed; }
  };

  unsigned bin_partition_seed() { return seed_pool_.current() + SEED_SALT; }

  unsigned get_bin_index(KeyType elm) {
    unsigned seed = bin_partition_seed();
    return hash(elm, seed) % (bch_length_ - 1) + 1;
  }

  unsigned get_group_id(KeyType elm) {
    unsigned seed = group_partition_seed();
    return hash(elm, seed) % initial_num_groups_;
  }

  unsigned group_partition_seed() { return seed_pool_.current(); }

  void calc_bch_params() {
    pbsutils::BestBchParam bch_param;
    pbsutils::PbsParam::bestBchParam(num_diffs_, avg_diffs_per_group_,
                                     max_rounds_, num_groups_when_bch_fail_,
                                     target_success_prob_, bch_param);
    bch_m_ = bch_param.n;
    bch_length_ = (1u << bch_param.n) - 1;
    bch_capacity_ = bch_param.t;
  }

  size_t get_loc(size_t i, size_t offset) {
    return get_bin_index(groups_[i][offset]);
  }

  ssize_t decode_single_group(unsigned char *other, unsigned gid,
                              std::vector<uint64_t> &differences) {
    size_t group_size = groups_[gid].size();

    auto &bitmap = bitmaps_[gid];
    auto &xor_single = xor_[gid];

    for (const auto elm : groups_[gid]) {
      // make sure no one goes into the bin with id 0
      // since minisketch does not support it right now
      size_t loc = get_bin_index(elm);
      bitmap[loc].flip();
      xor_single[loc] ^= elm;
    }

    minisketch *sketch = minisketch_create(bch_m_, 0, bch_capacity_);
    for (size_t k = 0; k < bitmap.size(); ++k) {
      if (bitmap[k]) minisketch_add_uint64(sketch, k);
    }
    minisketch *sketch_other = minisketch_create(bch_m_, 0, bch_capacity_);
    minisketch_deserialize(sketch_other, other);
    minisketch_merge(sketch, sketch_other);
    differences.resize(bch_capacity_);
    ssize_t p = minisketch_decode(sketch, bch_capacity_, &differences[0]);
    minisketch_destroy(sketch);
    minisketch_destroy(sketch_other);
    return p;
  }

  std::string encode_single_group(unsigned gid) {
    size_t group_size = groups_[gid].size();

    auto &bitmap = bitmaps_[gid];
    auto &xor_single = xor_[gid];

    auto seed = bin_partition_seed();
    for (const auto elm : groups_[gid]) {
      // make sure no one goes into the bin with id 0
      // since minisketch does not support it right now
      size_t loc = hash(elm, seed) % (bch_length_ - 1) + 1;
      bitmap[loc].flip();
      xor_single[loc] ^= elm;
    }

    minisketch *sketch = minisketch_create(bch_m_, 0, bch_capacity_);
    for (size_t k = 0; k < bitmap.size(); ++k) {
      if (bitmap[k]) minisketch_add_uint64(sketch, k);
    }
    std::string buffer(minisketch_serialized_size(sketch), ' ');
    minisketch_serialize(sketch, (unsigned char *)&buffer[0]);
    minisketch_destroy(sketch);
    return buffer;
  }

  static uint32_t hash(KeyType key, unsigned seed) {
    return XXH32(&key, sizeof(key), seed);
  }

  template <typename Iterator>
  void hash_partition(Iterator first, Iterator last) {
    size_t count = 0;
    for (auto it = first; it != last; ++it) {
      size_t gid = get_group_id(*it);
      groups_[gid].push_back(*it);
      group_ids_[count] = count++;
    }
  }

  size_t num_diffs_;
  unsigned max_rounds_;
  float avg_diffs_per_group_;
  unsigned num_groups_when_bch_fail_;
  unsigned current_round_;
  ssize_t remaining_num_groups_;
  size_t initial_num_groups_;

  double target_success_prob_;
  size_t bch_m_;
  size_t bch_length_;
  size_t bch_capacity_;

  SeedPool seed_pool_;
  std::vector<std::vector<KeyType>> groups_;
  std::vector<std::vector<KeyType>> xor_;
  std::vector<BitMap> bitmaps_;
  std::vector<size_t> group_ids_;
};

#endif  // PBS_H_

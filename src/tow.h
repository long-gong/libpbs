#ifndef __TOW_H__
#define __TOW_H__

#include <cassert>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <climits>
#include <cstdio>

/// Tug-of-war based set-difference estimator
/**
 *
 * @tparam MyHash    hash family
 */
template<typename MyHash>
struct TugOfWarHash {
  /**
   *
   * @param m              number of sketches
   * @param seed           random seed
   * @param max_range      largest possible seed for hash functions
   */
  TugOfWarHash(size_t m, unsigned seed, uint64_t max_range = UINT_MAX)
      : seed_(seed), m_(m), max_range_(max_range), hash_{} {
    generate_hashes();
  }

  /// apply this estimator
  /**
   *
   * @tparam Iterator          iterator type (iterator for std::vector, std::set, std::unordered_set and etc)
   * @param first, last        the range to apply this estimator to
   * @return                   tug-of-war sketches
   */
  template<typename Iterator>
  std::vector<int> apply(Iterator first, Iterator last) {
    std::vector<int> sketches(m_, 0);
    for (size_t i = 0; i < m_; ++i) {
      sketches[i] = apply<Iterator>(first, last, i);
    }
    return sketches;
  }

  /// apply this estimator
  /**
   *
   * @tparam Iterator          iterator type (iterator for std::unordered_map and etc)
   * @param first, last        the range to apply this estimator to
   * @return                   tug-of-war sketches
   */
  template<typename Iterator>
  std::vector<int> apply_key_value_pairs(Iterator first, Iterator last) {
    std::vector<int> sketches(m_, 0);
    for (size_t i = 0; i < m_; ++i) {
      sketches[i] = apply_key_value_pairs<Iterator>(first, last, i);
    }
    return sketches;
  }

  // number of sketches
  size_t num_sketches() const { return m_; }

 private:
  template<typename Iterator>
  int apply(Iterator first, Iterator last, size_t index) {
    int res = 0;
    for (auto it = first; it != last; ++it) {
      res += (hash_[index](*it) % 2 == 0) ? (-1) : (+1);
    }
    return res;
  }

  template<typename Iterator>
  int apply_key_value_pairs(Iterator first, Iterator last, size_t index) {
    int res = 0;
    for (auto it = first; it != last; ++it) {
      res += (hash_[index](it->first) % 2 == 0) ? (-1) : (+1);
    }
    return res;
  }

  /// generate hash functions
  void generate_hashes() {
    hash_.reserve(m_);
    std::mt19937_64 eg(seed_);
    std::uniform_int_distribution<unsigned> uniform(0, max_range_);
    std::unordered_set<unsigned> hash_seeds;
    hash_seeds.reserve(m_);
    while (hash_seeds.size() < m_)
      hash_seeds.insert(uniform(eg));

    for (auto seed : hash_seeds)
      hash_.emplace_back(seed);
  }

  unsigned seed_;
  size_t m_;
  uint64_t max_range_;
  std::vector<MyHash> hash_;
};

#endif // __TOW_H__


#ifndef BENCH_UTILS_H_
#define BENCH_UTILS_H_
#include <cstring>
#include <iterator>
#include <random>
#include <string>

namespace only_for_benchmark {
namespace {
const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
}

/**
 * @brief Generate a random string
 *
 * @tparam URBG                    random generator type
 * @param len                      string length
 * @param alphabet                 alphabet
 * @param alphabet_sz              size of the alphabet
 * @param g                        random generator
 * @return                         random string with a length of `len`
 */
template <class URBG>
inline std::string GenerateRandomString(size_t len, const char *alphabet,
                                        size_t alphabet_sz, URBG &&g) {
  typedef std::uniform_int_distribution<uint32_t> distr_t;
  distr_t D(0, alphabet_sz - 1);
  std::string res(len, '0');
  for (size_t i = 0; i < len; ++i) res[i] = alphabet[D(g)];
  return res;
}

/**
 * @brief Random key value pairs
 *
 * @tparam HashMapContainer             hash map container type (key should be
 * Integer and value should be std::string)
 * @tparam Integer                      key type
 * @param key_value_pairs               result
 * @param sz                            number of key value pairs to be
 * generated
 * @param value_size                    bytes of each value
 * @param seed                          random seed
 * @param alphabet                      alphabet
 */
template <typename HashMapContainer, typename Integer>
inline void GenerateKeyValuePairs(HashMapContainer &key_value_pairs, size_t sz,
                                  size_t value_size, uint32_t seed,
                                  const char *alphabet = alphanum) {
  std::mt19937_64 gen(seed);
  std::uniform_int_distribution<Integer> distribution(1);
  while (key_value_pairs.size() < sz) {
    auto key = distribution(gen);
    std::string value =
        GenerateRandomString(value_size, alphabet, strlen(alphabet), gen);
    key_value_pairs.insert({key, value});
  }
}

/**
 * @brief  Check whether two key value pairs are the same
 *
 * @tparam HashMapContainer     hash map container type
 * @param a                     key value pair a
 * @param b                     key value pair b
 * @return                      whether a == b
 */
template <typename HashMapContainer>
inline bool is_equal(const HashMapContainer &a, const HashMapContainer &b) {
  if (a.size() != b.size()) return false;

  for (const auto &kv : a) {
    if (!(b.count(kv.first) && b.at(kv.first) == kv.second)) return false;
  }
  return true;
}

}  // end namespace only_for_benchmark
#endif  // BENCH_UTILS_H_

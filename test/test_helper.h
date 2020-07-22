#ifndef TEST_HELPER_H_
#define TEST_HELPER_H_

namespace only_for_test {

template <typename UInt>
std::vector<UInt> GenerateSet(size_t sz) {
  std::random_device dec;
  std::mt19937_64 gen(dec());
  std::uniform_int_distribution<uint32_t > dist(1);
  size_t count = sz * 1.2;
  std::vector<UInt > temp;
  temp.reserve(count);
  for (size_t i = 0;i < count; ++ i) temp.push_back(dist(gen));
  std::sort(temp.begin(), temp.end());
  auto it = std::unique(temp.begin(), temp.end());
  temp.resize(std::distance(temp.begin(), it));
  std::shuffle(temp.begin(), temp.end(), gen);
  temp.resize(count);
  return temp;
}

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

} // namespace only_for_test
#endif //TEST_HELPER_H_

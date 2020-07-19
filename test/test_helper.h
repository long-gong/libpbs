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

} // namespace only_for_test
#endif //TEST_HELPER_H_

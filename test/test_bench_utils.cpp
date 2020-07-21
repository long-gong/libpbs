#include <gtest/gtest.h>
#include <fmt/format.h>
#include <tsl/ordered_map.h>

#include "bench_utils.h"

using namespace only_for_benchmark;

TEST(TestBenchUtils, RandomString) {
  std::string  alphabet("0123456789");
  std::random_device dev;
  std::mt19937_64 g(dev());
  size_t len = 10;
  auto res = GenerateRandomString(len, alphabet.c_str(), alphabet.size(), g);
  EXPECT_EQ(len, res.size());
  fmt::print("{}\n", res);
}

TEST(TestBenchUtils, RandomKeyValuePairs) {
  tsl::ordered_map<int, std::string> kvs;
  size_t sz = 10;
  size_t vsz = 4;
  unsigned seed = 20200721;

  GenerateKeyValuePairs<tsl::ordered_map<int, std::string>, int>(kvs, sz, vsz, seed);
  EXPECT_EQ(sz, kvs.size());

  auto print = [](const std::pair<int, std::string>& kv) {
    fmt::print("({}, {}) ", kv.first, kv.second);
  };
  std::for_each(kvs.cbegin(), kvs.cend(), print);

  tsl::ordered_map<int, std::string> kvs2;
  GenerateKeyValuePairs<tsl::ordered_map<int, std::string>, int>(kvs2, sz, vsz, seed);
  EXPECT_TRUE(is_equal(kvs, kvs2));
}


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


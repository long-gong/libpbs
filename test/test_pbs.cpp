#include <gtest/gtest.h>
#include "sketch.h"
#include <unordered_map>

using namespace libpbs;

TEST(PbsTest, TestDeterministic) {
  size_t d = 10;
  uint32_t start = 1000;
  ParityBitmapSketch alice(d);
  for (uint32_t e = start;e < start + d;++ e) alice.add(e);

  auto [encoding_msg, hint_msg] = alice.encode();

  EXPECT_EQ(nullptr, hint_msg);

  std::unordered_map<uint64_t, int> result;

  {
    ParityBitmapSketch bob(d);
    bob.encode();
    std::vector<uint64_t> xors, checksums;
    auto decoding_msg = bob.decode(*encoding_msg, xors, checksums);

    while (!alice.decodeCheck(*decoding_msg, xors, checksums)) {
      auto [enc, hint] = alice.encode();
      bob.encodeWithHint(hint->groups_with_exceptions.begin(), hint->groups_with_exceptions.end());
      xors.clear();
      checksums.clear();
      decoding_msg = bob.decode(*enc, xors, checksums);
      auto res = alice.differencesLastRound();
      for (auto elm: res) {
        if (result.count(elm)) result[elm] += 1;
        else result[elm] = 1;
      }
    }
    auto res = alice.differencesLastRound();
    for (auto elm: res) {
      if (result.count(elm)) result[elm] += 1;
      else result[elm] = 1;
    }

    auto print = [](const std::pair<uint64_t, int>& p) {
      std::cout << "(" << p.first << ", " << p.second << ") " ;
    };
    for (uint32_t e = start;e < start + d;++ e)
      EXPECT_TRUE(result.count(e) > 0 && (result[e] % 2 == 1));
    std::for_each(result.begin(), result.end(), print);
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


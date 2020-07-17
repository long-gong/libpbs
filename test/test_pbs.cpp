#include <gtest/gtest.h>
#include "pbs.h"
#include <unordered_map>

using namespace libpbs;

void DoDeterministicBobIsEmpty(size_t d, int verbose = 0) {
  uint32_t start = 1000;
  ParityBitmapSketch alice(d);
  for (uint32_t e = start; e < start + d; ++e) alice.add(e);

  auto[encoding_msg, hint_msg] = alice.encode();

  EXPECT_EQ(nullptr, hint_msg);

  std::unordered_map<uint64_t, int> result;

  {
    ParityBitmapSketch bob(d);
    bob.encode();
    std::vector<uint64_t> xors, checksums;
    auto decoding_msg = bob.decode(*encoding_msg, xors, checksums);

    while (!alice.decodeCheck(*decoding_msg, xors, checksums)) {
      auto[enc, hint] = alice.encode();
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
    if (verbose >= 1) {
      auto print = [](const std::pair<uint64_t, int> &p) {
        std::cout << "(" << p.first << ", " << p.second << ") ";
      };
      std::for_each(result.begin(), result.end(), print);
    }
    for (uint32_t e = start; e < start + d; ++e)
      EXPECT_TRUE(result.count(e) > 0 && (result[e] % 2 == 1));

    EXPECT_EQ(alice.rounds(), bob.rounds());
    if (verbose >= 0) printf("# of rounds: %lu (when d = %lu)\n", alice.rounds(), d);
  }
}

void DoDeterministicAliceIsEmpty(size_t d, int verbose = 0) {
  uint32_t start = 20200715u;
  ParityBitmapSketch alice(d);
  auto[encoding_msg, hint_msg] = alice.encode();

  EXPECT_EQ(nullptr, hint_msg);
  std::unordered_map<uint64_t, int> result;
  {
    ParityBitmapSketch bob(d);
    for (uint32_t e = start; e < start + d; ++e) bob.add(e);

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
      if (verbose >= 1) {
        printf("[d = %lu]: Round #%lu | # reconciled = %lu\n", d, alice.rounds(), res.size());
      }
      for (auto elm: res) {
        if (result.count(elm)) result[elm] += 1;
        else result[elm] = 1;
      }
    }
    auto res = alice.differencesLastRound();
    if (verbose >= 1) {
      printf("[d = %lu]: Round #%lu | # reconciled = %lu\n", d, alice.rounds(), res.size());
    }
    for (auto elm: res) {
      if (result.count(elm)) result[elm] += 1;
      else result[elm] = 1;
    }
    if (verbose >= 2) {
      auto print = [](const std::pair<uint64_t, int> &p) {
        std::cout << "(" << p.first << ", " << p.second << ") ";
      };
      std::for_each(result.begin(), result.end(), print);
    }
    for (uint32_t e = start; e < start + d; ++e) {
      EXPECT_TRUE(result.count(e) > 0) << "[d = " << d << "]: " << e << " should be in the result" << std::endl;
      if (result.count(e) > 0)
      EXPECT_TRUE((result[e] % 2 == 1))
                << "[d = " << d << "]: " << e << " should appear odd times in the result, but it appears " << result[e] << " times" << std::endl;
    }

    EXPECT_EQ(alice.rounds(), bob.rounds());
    if (verbose >= 0) printf("\n# of rounds: %lu (when d = %lu)\n", alice.rounds(), d);
  }
}

TEST(PbsTest, DeterministicBobIsEmpty) {
  for (size_t d: {10, 100, 1000, 10000, 100000})
    DoDeterministicBobIsEmpty(d);
}

TEST(PbsTest, DeterministicAliceIsEmpty) {
  for (size_t d: {10, 100, 1000, 10000, 100000})
    DoDeterministicAliceIsEmpty(d);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


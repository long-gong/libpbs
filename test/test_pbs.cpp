#include <gtest/gtest.h>

#include <random>
#include <unordered_map>

#include "pbs.h"
#include "test_helper.h"

using namespace libpbs;
using namespace only_for_test;

std::pair<std::vector<uint32_t>, std::vector<uint32_t>> GenerateRandomSetPair(
    size_t d, float ratio_a, size_t intersection_sz = 0);
std::pair<std::vector<uint32_t>, std::vector<uint32_t>>
GenerateDeterministicSetPair(size_t d, float ratio_a,
                             size_t intersection_sz = 0, uint32_t start = 1000);

void DoDeterministicBobIsEmpty(size_t d, int verbose = 0);
void DoDeterministicAliceIsEmpty(size_t d, int verbose = 0);
void DoDeterministicBothNotEmpty(size_t d, float ratio_a,
                                 size_t intersection_sz = 0, int verbose = 0);

void DoRandomAllInOne(size_t d, float ratio_a,
                      size_t intersection_sz = 0, int verbose = 0);


TEST(PbsTest, DeterministicBobIsEmpty) {
  for (size_t d : {10, 100, 1000, 10000, 100000}) DoDeterministicBobIsEmpty(d);
}

TEST(PbsTest, DeterministicAliceIsEmpty) {
  for (size_t d : {10, 100, 1000, 10000, 100000})
    DoDeterministicAliceIsEmpty(d);
}

TEST(PbsTest, DeterministicBothNotEmptyNoIntersection) {
  for (float ratio_a = 0.1; ratio_a < 1.0; ratio_a += 0.1) {
    for (size_t d : {10, 100, 1000, 10000, 100000})
      DoDeterministicBothNotEmpty(d, ratio_a);
  }
}

TEST(PbsTest, DeterministicBothNotEmptyWithIntersection) {
  float ratio_a = 0.5;
  for (size_t intersection : {10, 100, 1000, 10000, 100000}) {
    for (size_t d : {10, 100, 1000, 10000, 100000})
      DoDeterministicBothNotEmpty(d, ratio_a, intersection);
  }
}

TEST(PbsTest, DeterministicNoIntersection) {
  for (float ratio_a = 0; ratio_a <= 1.0; ratio_a += 0.1) {
    for (size_t d : {10, 100, 1000, 10000, 100000})
      DoRandomAllInOne(d, ratio_a);
  }
}

TEST(PbsTest, DeterministicBothWithIntersection) {
  float ratio_a = 0.5;
  for (size_t intersection : {10, 100, 1000, 10000, 100000}) {
    for (size_t d : {10, 100, 1000, 10000, 100000})
      DoRandomAllInOne(d, ratio_a, intersection);
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

std::pair<std::vector<uint32_t>, std::vector<uint32_t>> GenerateRandomSetPair(
    size_t d, float ratio_a, size_t intersection_sz) {
  assert(ratio_a >= 0 && ratio_a <= 1);
  auto da = static_cast<size_t>(std::round(ratio_a * d));
  auto db = d - da;

  std::vector<uint32_t> sa, sb;
  sa.reserve(da + intersection_sz);
  sb.reserve(db + intersection_sz);

  auto temp = GenerateSet<uint32_t>(d + intersection_sz);
  size_t index = 0;
  for (size_t i = 0; i < intersection_sz; ++i) {
    sa.push_back(temp[index]);
    sb.push_back(temp[index]);
    index++;
  }
  for (size_t i = 0; i < d; ++i) {
    if (i < da)
      sa.push_back(temp[index]);
    else
      sb.push_back(temp[index]);
    index++;
  }

  return {sa, sb};
}

std::pair<std::vector<uint32_t>, std::vector<uint32_t>>
GenerateDeterministicSetPair(size_t d, float ratio_a, size_t intersection_sz,
                             uint32_t start) {
  assert(ratio_a >= 0 && ratio_a <= 1);
  auto da = static_cast<size_t>(std::round(ratio_a * d));
  auto db = d - da;

  std::vector<uint32_t> sa, sb;
  sa.reserve(da + intersection_sz);
  sb.reserve(db + intersection_sz);

  size_t elm = start;
  for (size_t i = 0; i < d; ++i) {
    if (i < da)
      sa.push_back(elm);
    else
      sb.push_back(elm);
    elm++;
  }
  for (size_t i = 0; i < intersection_sz; ++i) {
    sa.push_back(elm);
    sb.push_back(elm);
    elm++;
  }

  return {sa, sb};
}


void DoRandomAllInOne(size_t d, float ratio_a,size_t intersection_sz, int verbose) {
  ParityBitmapSketch alice(d);
  auto [sa, sb] = GenerateRandomSetPair(d, ratio_a, intersection_sz);
  for (uint32_t e : sa) alice.add(e);

  auto [encoding_msg, hint_msg] = alice.encode();
  EXPECT_EQ(nullptr, hint_msg);
  std::unordered_map<uint64_t, int> result;
  {
    ParityBitmapSketch bob(d);
    for (uint32_t e : sb) bob.add(e);

    bob.encode();
    std::vector<uint64_t> xors, checksums;
    auto decoding_msg = bob.decode(*encoding_msg, xors, checksums);

    while (!alice.decodeCheck(*decoding_msg, xors, checksums)) {
      auto [enc, hint] = alice.encode();
      bob.encodeWithHint(hint->groups_with_exceptions.begin(),
                         hint->groups_with_exceptions.end());
      xors.clear();
      checksums.clear();
      decoding_msg = bob.decode(*enc, xors, checksums);
      auto res = alice.differencesLastRound();
      for (auto elm : res) {
        if (result.count(elm))
          result[elm] += 1;
        else
          result[elm] = 1;
      }
    }
    auto res = alice.differencesLastRound();
    for (auto elm : res) {
      if (result.count(elm))
        result[elm] += 1;
      else
        result[elm] = 1;
    }
    if (verbose >= 1) {
      auto print = [](const std::pair<uint64_t, int> &p) {
        std::cout << "(" << p.first << ", " << p.second << ") ";
      };
      std::for_each(result.begin(), result.end(), print);
    }

    for (uint32_t k = intersection_sz;k < sa.size(); ++ k) {
      auto e = sa[k];
      EXPECT_TRUE(result.count(e) > 0 && (result[e] % 2 == 1));
    }

    for (uint32_t k = intersection_sz;k < sb.size(); ++ k) {
      auto e = sb[k];
      EXPECT_TRUE(result.count(e) > 0 && (result[e] % 2 == 1));
    }

    EXPECT_EQ(alice.rounds(), bob.rounds());
    if (verbose >= 0)
      printf("# of rounds: %lu (when d = %lu)\n", alice.rounds(), d);
  }
}

void DoDeterministicBothNotEmpty(size_t d, float ratio_a,
                                 size_t intersection_sz, int verbose) {
  uint32_t start = 1000;
  ParityBitmapSketch alice(d);
  auto [sa, sb] =
      GenerateDeterministicSetPair(d, ratio_a, intersection_sz, start);
  for (uint32_t e : sa) alice.add(e);

  auto [encoding_msg, hint_msg] = alice.encode();
  EXPECT_EQ(nullptr, hint_msg);
  std::unordered_map<uint64_t, int> result;
  {
    ParityBitmapSketch bob(d);
    for (uint32_t e : sb) bob.add(e);

    bob.encode();
    std::vector<uint64_t> xors, checksums;
    auto decoding_msg = bob.decode(*encoding_msg, xors, checksums);

    while (!alice.decodeCheck(*decoding_msg, xors, checksums)) {
      auto [enc, hint] = alice.encode();
      bob.encodeWithHint(hint->groups_with_exceptions.begin(),
                         hint->groups_with_exceptions.end());
      xors.clear();
      checksums.clear();
      decoding_msg = bob.decode(*enc, xors, checksums);
      auto res = alice.differencesLastRound();
      for (auto elm : res) {
        if (result.count(elm))
          result[elm] += 1;
        else
          result[elm] = 1;
      }
    }
    auto res = alice.differencesLastRound();
    for (auto elm : res) {
      if (result.count(elm))
        result[elm] += 1;
      else
        result[elm] = 1;
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
    if (verbose >= 0)
      printf("# of rounds: %lu (when d = %lu)\n", alice.rounds(), d);
  }
}

void DoDeterministicBobIsEmpty(size_t d, int verbose) {
  uint32_t start = 1000;
  ParityBitmapSketch alice(d);
  for (uint32_t e = start; e < start + d; ++e) alice.add(e);
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
      bob.encodeWithHint(hint->groups_with_exceptions.begin(),
                         hint->groups_with_exceptions.end());
      xors.clear();
      checksums.clear();
      decoding_msg = bob.decode(*enc, xors, checksums);
      auto res = alice.differencesLastRound();
      for (auto elm : res) {
        if (result.count(elm))
          result[elm] += 1;
        else
          result[elm] = 1;
      }
    }
    auto res = alice.differencesLastRound();
    for (auto elm : res) {
      if (result.count(elm))
        result[elm] += 1;
      else
        result[elm] = 1;
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
    if (verbose >= 0)
      printf("# of rounds: %lu (when d = %lu)\n", alice.rounds(), d);
  }
}

void DoDeterministicAliceIsEmpty(size_t d, int verbose) {
  uint32_t start = 20200715u;
  ParityBitmapSketch alice(d);
  auto [encoding_msg, hint_msg] = alice.encode();

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
      bob.encodeWithHint(hint->groups_with_exceptions.begin(),
                         hint->groups_with_exceptions.end());
      xors.clear();
      checksums.clear();
      decoding_msg = bob.decode(*enc, xors, checksums);
      auto res = alice.differencesLastRound();
      if (verbose >= 1) {
        printf("[d = %lu]: Round #%lu | # reconciled = %lu\n", d,
               alice.rounds(), res.size());
      }
      for (auto elm : res) {
        if (result.count(elm))
          result[elm] += 1;
        else
          result[elm] = 1;
      }
    }
    auto res = alice.differencesLastRound();
    if (verbose >= 1) {
      printf("[d = %lu]: Round #%lu | # reconciled = %lu\n", d, alice.rounds(),
             res.size());
    }
    for (auto elm : res) {
      if (result.count(elm))
        result[elm] += 1;
      else
        result[elm] = 1;
    }
    if (verbose >= 2) {
      auto print = [](const std::pair<uint64_t, int> &p) {
        std::cout << "(" << p.first << ", " << p.second << ") ";
      };
      std::for_each(result.begin(), result.end(), print);
    }
    for (uint32_t e = start; e < start + d; ++e) {
      EXPECT_TRUE(result.count(e) > 0)
          << "[d = " << d << "]: " << e << " should be in the result"
          << std::endl;
      if (result.count(e) > 0)
        EXPECT_TRUE((result[e] % 2 == 1))
            << "[d = " << d << "]: " << e
            << " should appear odd times in the result, but it appears "
            << result[e] << " times" << std::endl;
    }

    EXPECT_EQ(alice.rounds(), bob.rounds());
    if (verbose >= 0)
      printf("\n# of rounds: %lu (when d = %lu)\n", alice.rounds(), d);
  }
}


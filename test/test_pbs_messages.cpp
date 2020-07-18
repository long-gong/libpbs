#include <fmt/format.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <random>

#include "pbs_decoding_message.h"
#include "pbs_encoding_hint_message.h"
#include "pbs_encoding_message.h"

using namespace libpbs;

/**
 *  EncodingMessage: "good cases" -- bch_m * bch_t * num_groups % 8 == 0
 */
TEST(PbsMessagesTest, EncodingMessageSingleGroupGoodCase) {
  size_t bch_m = 12, bch_t = 4, num_groups = 1;
  PbsEncodingMessage message(bch_m, bch_t, num_groups);
  auto sketch_a = message.getSketch(0);
  for (int i : {1, 2, 3, 5}) {
    minisketch_add_uint64(sketch_a, i);
  }

  // Should be 6 bytes
  auto ss = message.serializedSize();
  EXPECT_EQ(ss, 6);
  std::string buffer(ss, 0);
  message.write((uint8_t *)&buffer[0]);

  // create another message (MUST use the same parameters)
  PbsEncodingMessage message2(bch_m, bch_t, num_groups);
  // parse the buffer
  auto psz = message2.parse((const uint8_t *)&buffer[0], buffer.size());
  // parsed size should equal buffer size
  EXPECT_EQ(psz, buffer.size());

  // original sketch
  auto sa = message.getSketch(0);
  // parsed sketch
  auto sb = message2.getSketch(0);
  minisketch_merge(sa, sb);

  std::vector<uint64_t> differences(bch_t, 0);
  ssize_t num_differences = minisketch_decode(sa, bch_t, &differences[0]);
  // should be the same (i.e., no difference)
  EXPECT_EQ(num_differences, 0);
}

TEST(PbsMessagesTest, EncodingMessageSingleGroupBadCase) {
  size_t bch_m = 6, bch_t = 6, num_groups = 1;
  PbsEncodingMessage message(bch_m, bch_t, num_groups);
  auto sketch_a = message.getSketch(0);
  for (int i : {1, 2, 3, 5}) {
    minisketch_add_uint64(sketch_a, i);
  }
  auto ss = message.serializedSize();
  // should be 5 bytes
  EXPECT_EQ(ss, 5);
  std::string buffer(ss, 0);
  message.write((uint8_t *)&buffer[0]);

  PbsEncodingMessage message2(bch_m, bch_t, num_groups);
  auto psz = message2.parse((const uint8_t *)&buffer[0], buffer.size());
  EXPECT_EQ(psz, buffer.size());

  auto sa = message.getSketch(0);
  auto sb = message2.getSketch(0);
  minisketch_merge(sa, sb);

  std::vector<uint64_t> differences(bch_t, 0);
  ssize_t num_differences = minisketch_decode(sa, bch_t, &differences[0]);
  // should be exactly the same
  EXPECT_EQ(num_differences, 0);
}

TEST(PbsMessagesTest, EncodingMessageMultipleGroupGoodCase) {
  size_t bch_m = 7, bch_t = 7, num_groups = 8;

  PbsEncodingMessage message(bch_m, bch_t, num_groups);
  auto sketch_a = message.getSketch(0);
  for (size_t g = 0; g < num_groups; ++g) {
    for (int i : {1, 2, 3, 5}) {
      minisketch_add_uint64(sketch_a, i + g);
    }
  }

  // Should be 49 bytes
  auto ss = message.serializedSize();
  EXPECT_EQ(ss, 49);
  std::string buffer(ss, 0);
  message.write((uint8_t *)&buffer[0]);

  // create another message (MUST use the same parameters)
  PbsEncodingMessage message2(bch_m, bch_t, num_groups);
  // parse the buffer
  auto psz = message2.parse((const uint8_t *)&buffer[0], buffer.size());
  // parsed size should equal buffer size
  EXPECT_EQ(psz, buffer.size());

  for (size_t g = 0; g < num_groups; ++g) {
    // original sketch
    auto sa = message.getSketch(g);
    // parsed sketch
    auto sb = message2.getSketch(g);
    minisketch_merge(sa, sb);

    std::vector<uint64_t> differences(bch_t, 0);
    ssize_t num_differences = minisketch_decode(sa, bch_t, &differences[0]);
    // should be exactly the same
    EXPECT_EQ(num_differences, 0);
  }
}

TEST(PbsMessagesTest, EncodingMessageMultipleGroupBadCase) {
  size_t bch_m = 12, bch_t = 7, num_groups = 5;
  PbsEncodingMessage message(bch_m, bch_t, num_groups);

  for (size_t g = 0; g < num_groups; ++g) {
    auto sketch_a = message.getSketch(g);
    for (int i = 3000; i < 3010; ++i) {
      minisketch_add_uint64(sketch_a, g);
    }
  }

  auto ss = message.serializedSize();
  // should be 53 bytes
  EXPECT_EQ(ss, 53);
  std::vector<uint8_t> buffer(ss, 0);
  message.write(&buffer[0]);

  PbsEncodingMessage message2(bch_m, bch_t, num_groups);

  auto psz = message2.parse(&buffer[0], buffer.size());
  EXPECT_EQ(psz, buffer.size());

  for (size_t g = 0; g < num_groups; ++g) {
    auto sa = message2.getSketch(g);
    auto sb = message2.getSketch(g);
    minisketch_merge(sa, sb);
    std::vector<uint64_t> differences(bch_t, 0);
    ssize_t num_differences = minisketch_decode(sa, bch_t, &differences[0]);
    EXPECT_EQ(num_differences, 0);
  }
}

TEST(PbsMessagesTest, DecodingMessageWriteThenParse) {
  size_t bch_m = 12, bch_t = 7, num_groups = 3;
  PbsDecodingMessage message(bch_m, bch_t, num_groups);
  message.decoded_num_differences = {3, 2, -1};
  message.decoded_differences = {1, 2, 3, 19, 43};
  size_t ss = message.serializedSize();
  std::vector<uint8_t> buffer(ss);
  auto w_sz = message.write(&buffer[0]);
  EXPECT_EQ(buffer.size(), w_sz);

  PbsDecodingMessage message1(bch_m, bch_t, num_groups);
  auto r_sz = message1.parse(&buffer[0], buffer.size());
  EXPECT_EQ(buffer.size(), r_sz);
  EXPECT_EQ(message1.decoded_num_differences, message.decoded_num_differences);
  EXPECT_EQ(message1.decoded_differences, message.decoded_differences);
}

TEST(PbsMessagesTest, DecodingMessageSetWithSucceed) {
  size_t bch_m = 12, bch_t = 7, num_groups = 5;
  PbsDecodingMessage message(bch_m, bch_t, num_groups);

  std::vector<minisketch *> sketches(num_groups * 2);
  // allocate memory
  for (auto &sketch : sketches) sketch = minisketch_create(bch_m, 0, bch_t);

  std::vector<std::vector<uint64_t>> diffs(num_groups);
  std::vector<ssize_t> num_diffs(num_groups);
  for (size_t g = 0; g < num_groups; ++g) {
    for (size_t k = 0; k < g + 1; ++k) diffs[g].push_back(300 + k);
    for (auto elm : diffs[g]) minisketch_add_uint64(sketches[g], elm);
    num_diffs[g] = g + 1;
  }

  message.setWith(sketches.begin(), sketches.begin() + num_groups,
                  sketches.cbegin() + num_groups);
  EXPECT_EQ(message.decoded_num_differences, num_diffs);

  size_t offset = 0;
  for (size_t g = 0; g < num_groups; ++g) {
    std::sort(message.decoded_differences.begin() + offset,
              message.decoded_differences.begin() + offset + diffs[g].size());
    EXPECT_TRUE(std::equal(diffs[g].begin(), diffs[g].end(),
                           message.decoded_differences.begin() + offset));
    offset += diffs[g].size();
  }

  // release memory
  for (auto &sketch : sketches) minisketch_destroy(sketch);
}

/**
 * NOTE: This test might fail, because it is possible for BCH codes
 *       to decode "something" out (especially when t is small) when
 *       the actual number of bit errors are larger than t (error-correcting
 *       capacity). This probability is extremely low, when t >= 9.
 *
 *  For more details, please refer to the following paper:
 *
 *    Min-Goo Kim and Jae Hong Lee, "Undetected error probabilities of binary
 *    primitive BCH codes for both error correction and detection," in IEEE
 *    Transactions on Communications, vol. 44, no. 5, pp. 575-580, May 1996.
 *
 */
TEST(PbsMessagesTest, DecodingMessageSetWithFailed) {
  size_t bch_m = 10, bch_t = 9, num_groups = 5;
  PbsDecodingMessage message(bch_m, bch_t, num_groups);

  std::vector<minisketch *> sketches(num_groups * 2);
  // allocate memory
  for (auto &sketch : sketches) sketch = minisketch_create(bch_m, 0, bch_t);

  std::vector<uint64_t> tested_numbers;
  for (size_t g = 0; g < num_groups; ++g) {
    for (size_t k = 0; k < bch_t + 1; ++k) {
      tested_numbers.push_back(30 + 100 * g + k);
      minisketch_add_uint64(sketches[g], tested_numbers.back());
    }
  }

  message.setWith(sketches.begin(), sketches.begin() + num_groups,
                  sketches.cbegin() + num_groups);

  // all should fail
  EXPECT_TRUE(std::all_of(message.decoded_num_differences.begin(),
                          message.decoded_num_differences.end(),
                          [](const ssize_t x) { return x == -1; }));
  // no decoded elements
  EXPECT_TRUE(message.decoded_differences.empty());

  // release memory
  for (auto &sketch : sketches) minisketch_destroy(sketch);
}

TEST(PbsMessagesTest, DecodingMessageSetWithMixed) {
  size_t bch_m = 12, bch_t = 11, num_groups = 5;
  PbsDecodingMessage message(bch_m, bch_t, num_groups);

  std::vector<minisketch *> sketches(num_groups * 2);
  // allocate memory
  for (auto &sketch : sketches) sketch = minisketch_create(bch_m, 0, bch_t);

  std::vector<size_t> num_diffs;
  std::vector<std::vector<uint64_t>> diffs(num_groups);

  std::random_device dev;
  std::mt19937_64 gen(dev());
  std::uniform_int_distribution<> dist(0, bch_t * 3 / 2);
  for (size_t g = 0; g < num_groups; ++g) {
    size_t d = dist(gen);
    num_diffs.push_back(d);
    for (size_t k = 0; k < d; ++k) {
      diffs[g].push_back(100 * g + k + 1);
      minisketch_add_uint64(sketches[g], diffs[g].back());
    }
  }

  message.setWith(sketches.begin(), sketches.begin() + num_groups,
                  sketches.cbegin() + num_groups);

  size_t offset = 0;
  for (size_t g = 0;g < num_groups;++g) {
    if (num_diffs[g] > bch_t) {
      EXPECT_EQ(message.decoded_num_differences[g], -1);
    } else {
      std::sort(message.decoded_differences.begin() + offset,
                message.decoded_differences.begin() + offset + diffs[g].size());
      EXPECT_TRUE(std::equal(diffs[g].begin(), diffs[g].end(),
                             message.decoded_differences.begin() + offset));
      offset += diffs[g].size();
    }
  }
  // release memory
  for (auto &sketch : sketches) minisketch_destroy(sketch);
}

TEST(PbsMessagesTest, DecodingHintMessage) {
  size_t num_groups = 215;
  PbsEncodingHintMessage decoding_hint_message(num_groups);

  std::vector<uint32_t> test_ids = {1, 9, 101};
  for (uint32_t gid : test_ids) decoding_hint_message.addGroupId(gid);

  auto ss = decoding_hint_message.serializedSize();
  std::vector<uint8_t> buffer(ss, 0);
  decoding_hint_message.write(&buffer[0]);
  {
    PbsEncodingHintMessage decoding_hint_message1(num_groups);
    auto psz = decoding_hint_message1.parse(&buffer[0], buffer.size());
    EXPECT_EQ(buffer.size(), psz);
    EXPECT_EQ(decoding_hint_message.groups_with_exceptions, test_ids);
  }
}

// TEST(PbsMessagesTest, DecodingHintMessage) {
//  PbsDecodingMessage message(12, 7, 3);
//  message.decoded_num_differences = {3, 2, -1};
//  message.decoded_differences = {1, 2, 3, 19, 43};
//
//  PbsEncodingHintMessage decoding_hint_message(message);
//  decoding_hint_message.exception_i_flags = {0, 1, 0};
//  decoding_hint_message.exception_ii_counts = {1, 1, 0};
//  decoding_hint_message.exception_ii_bins = {2, 1};
//
//  auto ss = decoding_hint_message.serializedSize();
//  std::vector<uint8_t> buffer(ss, 0);
//  decoding_hint_message.write(&buffer[0]);
//  {
//    PbsEncodingHintMessage decoding_hint_message1(message);
//    auto psz = decoding_hint_message1.parse(&buffer[0], buffer.size());
//
//    EXPECT_EQ(buffer.size(), psz);
//
//    EXPECT_EQ(decoding_hint_message.exception_i_flags,
//              decoding_hint_message1.exception_i_flags);
//    EXPECT_EQ(decoding_hint_message.exception_ii_counts,
//              decoding_hint_message1.exception_ii_counts);
//    EXPECT_EQ(decoding_hint_message.exception_ii_bins,
//              decoding_hint_message1.exception_ii_bins);
//  }
//}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}

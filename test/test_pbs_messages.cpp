#include <gtest/gtest.h>

#include "pbs_encoding_hint_message.h"
#include "pbs_decoding_message.h"
#include "pbs_encoding_message.h"

using namespace libpbs;

TEST(PbsMessagesTest, EncodingMessage) {
  PbsEncodingMessage message(12, 7, 2);

  auto sketch_a = message.getSketch(0);
  for (int i = 3000; i < 3010; ++i) {
    minisketch_add_uint64(sketch_a, i);
  }

  minisketch *sketch_b = message.getSketch(1);
  for (int i = 3002; i < 3012; ++i) {
    minisketch_add_uint64(sketch_b, i);
  }

  auto ss = message.serializedSize();

  std::vector<uint8_t> buffer(ss, 0);

  message.write(&buffer[0]);

  // receiver side
  {
    PbsEncodingMessage message2(12, 7, 2);

    message2.parse(&buffer[0], buffer.size());

    auto sa = message2.getSketch(0);
    auto sb = message2.getSketch(1);
    minisketch_merge(sa, sb);

    std::vector<uint64_t> differences(7, 0);
    ssize_t num_differences = minisketch_decode(sa, 7, &differences[0]);

    EXPECT_EQ(num_differences, 4);
    differences.resize(num_differences);
    std::sort(differences.begin(), differences.end());
    auto expected = std::vector<uint64_t>{3000, 3001, 3010, 3011};
    EXPECT_EQ(expected, differences);
  }
}

TEST(PbsMessagesTest, DecodingMessage) {
  PbsDecodingMessage message(12, 7, 3);
  message.decoded_num_differences = {3, 2, -1};
  message.decoded_differences = {1, 2, 3, 19, 43};
  size_t ss = message.serializedSize();
  std::vector<uint8_t> buffer(ss);
  auto w_sz = message.write(&buffer[0]);

  EXPECT_EQ(buffer.size(), w_sz);
  {
    PbsDecodingMessage message1(12, 7, 3);
    auto r_sz = message1.parse(&buffer[0], buffer.size());
    EXPECT_EQ(buffer.size(), r_sz);
    EXPECT_EQ(message1.decoded_num_differences,
              message.decoded_num_differences);
    EXPECT_EQ(message1.decoded_differences, message.decoded_differences);
  }
}

TEST(PbsMessagesTest, DecodingHintMessage) {
  size_t num_groups = 215;
  PbsEncodingHintMessage decoding_hint_message(num_groups);

  std::vector<uint32_t> test_ids = {1, 9, 101};
  for (uint32_t gid: test_ids) decoding_hint_message.addGroupId(gid);

  auto ss = decoding_hint_message.serializedSize();
  std::vector<uint8_t> buffer(ss, 0);
  decoding_hint_message.write(&buffer[0]);
  {
    PbsEncodingHintMessage decoding_hint_message1(num_groups);
    auto psz = decoding_hint_message1.parse(&buffer[0], buffer.size());
    EXPECT_EQ(buffer.size(), psz);
    EXPECT_EQ(decoding_hint_message.groups_with_exceptions,
              test_ids);
  }
}

//TEST(PbsMessagesTest, DecodingHintMessage) {
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

/**
 * @file pbs_encoding_hint_message.h
 * @author Long Gong <long.github@gmail.com>
 * @brief The PBS Encoding Hint Message.
 * @version 0.1
 * @date 2020-07-06
 *
 * @copyright Copyright (c) 2020
 *
 */
#ifndef _PBS_DECODING_HINT_MESSAGE_H_
#define _PBS_DECODING_HINT_MESSAGE_H_

#include <numeric>
#include <stdexcept>

#include "pbs_decoding_message.h"
#include "pbs_message.h"

namespace libpbs {

namespace {
constexpr std::size_t BITS_PER_BLOCK = 8u;
}

/**
 * @brief The PbsEncodingHintMessage class.
 *
 * It tells whether exceptions (exception I or II) happens in the corresponding
 * PbsDecodingMessage instance.
 *
 * About the design choice
 * -----------------------
 * In the current implementation, we only encode the IDs of groups where
 * exceptions happened, each of which has a length of log2 max_range bits.
 * According to our analysis, the probability of the event that a group becomes
 * the ideal case is around 0.96. That is the exception happens with a
 * probability of roughly 0.04. Therefore, the average number of bits using this
 * approach is around 0.04 * max_range * log2 max_range. An alternative approach
 * is to use a bitmap with a length of max_range. To make this approach better
 * than the one we currently use, we should have:
 *             0.04 * max_range * log2 max_range > max_range  ==> max_range >
 * 2^25 In most applications, max_range should be less than 2^25.
 *
 *
 *
 * Note that need pay special attention to the cases where bits_each <= 4.
 * Currently, we shift gid by 1 so whenever we encounter a zero, it means
 * termination. Actually, we can also use the property that the group IDs are in
 * increasing order (can save 1 bit per each group ID).
 *
 * Note that there is no need for the hint of BCH decoding failure.
 *
 */
class PbsEncodingHintMessage : public PbsMessage {
 public:
  /**
   * @brief   Constructor
   *
   * Note that the maximum range should be set as the largest
   * possible value of all groups not just those groups where
   * exceptions (I) or (II) happened. Since this message will
   * not be written into the serialized message.
   *
   * @param max_range   maximum range for group ids
   */
  explicit PbsEncodingHintMessage(size_t max_range)
      : PbsMessage(PbsMessageType::ENCODING_HINT),
        max_range(max_range),
        bits_each(utils::CeilLog2(max_range)),
        groups_with_exceptions() {}

  ~PbsEncodingHintMessage() override = default;
  // maximum range of group IDs
  std::size_t max_range;
  // bits for each ID
  std::size_t bits_each;
  // IDs of groups where exception (I) or (II) happened
  std::vector<uint32_t> groups_with_exceptions;

  /**
   * @brief Parse from buffer.
   *
   * @param from            buffer to parse
   * @param msg_sz          buffer size (in bytes)
   * @return                sizes being parsed, returns -1 if failed
   */
  ssize_t parse(const uint8_t *from, size_t msg_sz) override {
    if (msg_sz == 0) return 0;
    utils::BitReader reader(from);
    auto count = msg_sz * 8 / bits_each;

    groups_with_exceptions.reserve(count);
    for (size_t i = 0; i < count; ++i) {
      auto gid = reader.Read<uint32_t>(bits_each);
      if (gid == 0 && i > 0) break;
      if (gid >= max_range) throw std::out_of_range("gid is out of range");
      groups_with_exceptions.push_back(gid);
    }

    return msg_sz;
  }

  /**
   * @brief Add a group ID
   *
   * @param gid     group ID
   */
  void addGroupId(uint32_t gid) {
    if (gid >= max_range) throw std::out_of_range("gid is out of range");
    groups_with_exceptions.push_back(gid);
  }

  /**
   * @brief  Write to buffer.
   *
   * @param to    buffer to write
   * @return      number of written bytes
   */
  ssize_t write(uint8_t *to) const override {
    auto sz = serializedSize();
    std::fill(to, to + sz, 0);
    utils::BitWriter writer(to);

    for (uint32_t gid : groups_with_exceptions)
      writer.Write<uint32_t>(gid, bits_each);
    // tell writer that I am completed
    writer.Flush();
    return sz;
  }

  /**
   * @brief Get serialized size (in bytes)
   *
   * @return    serialized size
   */
  [[nodiscard]] ssize_t serializedSize() const override {
    auto total_bits = groups_with_exceptions.size() * bits_each;
    return utils::Bits2Bytes(total_bits);
  }
};
}  // end namespace libpbs

#endif  //_PBS_DECODING_HINT_MESSAGE_H_

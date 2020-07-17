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
#include <cassert>

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
        groups_with_exceptions() {}
  ~PbsEncodingHintMessage() override = default;
  // maximum range of group IDs
  std::size_t max_range;
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
    for (size_t i = 0; i < max_range; ++i)
      if ((from[block_index(i)] & bit_mask(i)) != 0u) groups_with_exceptions.push_back(i);
    return msg_sz;
  }

  /**
   * @brief Add a group ID
   *
   * @param gid     group ID
   */
  void addGroupId(uint32_t gid) {
    assert (gid < max_range);
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
    memset(to, 0, sz);
    for (uint32_t gid: groups_with_exceptions)
      to[block_index(gid)] |= bit_mask(gid);
  }

  /**
   * @brief Get serialized size (in bytes)
   *
   * @return    serialized size
   */
  [[nodiscard]] ssize_t serializedSize() const override {
    return utils::Bits2Bytes(max_range);
  }

 private:
  /**
   * @brief Get the block index.
   *
   * @param pos      bit position to obtain the block index for
   * @return         block index
   */
  [[nodiscard]] constexpr size_t block_index(size_t pos) const noexcept {
    return pos / BITS_PER_BLOCK;
  }

  /**
   * @brief Get bit mask.
   *
   * @param pos       bit position to get the bit mask for
   * @return          bit mask
   */
  [[nodiscard]] constexpr uint8_t bit_mask(size_t pos) const noexcept {
    return ((uint8_t) 1) << bit_index(pos);
  }

  /**
   * @brief Get bit index within a block
   *
   * @param pos      bit position to obtain the bit index for
   * @return         bit index
   */
  [[nodiscard]] constexpr size_t bit_index(size_t pos) const noexcept {
    return pos % BITS_PER_BLOCK;
  }
};
}  // end namespace libpbs

#endif  //_PBS_DECODING_HINT_MESSAGE_H_

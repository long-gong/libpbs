/**
 * @file pbs_decoding_message.h
 * @author Long Gong <long.github@gmail.com>
 * @brief The PBS Decoding Message.
 * @version 0.1
 * @date 2020-07-06
 *
 * @copyright Copyright (c) 2020
 *
 */
#ifndef _PBS_DECODING_MESSAGE_H_
#define _PBS_DECODING_MESSAGE_H_

#if __cplusplus < 201703L
#error This library needs at least a C++17 compliant compiler
#endif

#include <minisketch.h>

#include <cmath>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <vector>

#include "bit_utils.h"
#include "pbs_message.h"

namespace libpbs {

/**
 * \brief  The PbsDecodingMessage class.
 *
 * Each PbsDecodingMessage consists of two parts:
 * `decoded_num_differences` and `decoded_differences`.
 * The former records the decoded number of differences in each subset/group, and
 * the latter records all differences (i.e., locations where two bitmaps in each subset
 * pair or group pair differ). Although storing (number of differences, differences) per group seems
 * better for readability, we choose to separate this two parts for increasing space efficiency,
 * i.e., avoiding paying overhead of std::vector for differences for each group.
 *
 * Note that number of differences being -1 indicates that the `capacity` is not enough
 * for the BCH decoder to correctly decode the encoded message.
 *
 */
class PbsDecodingMessage : public PbsMessage {
 public:
  // field size
  uint32_t field_sz;
  // capacity, how many differences (different bit positions between the two
  // bitmaps) can this resulting correctly pinpoint
  uint32_t capacity;
  // number of groups
  size_t num_groups;
  // size of each set difference
  uint32_t sizeof_each_d;
  // number to indicate that BCH decoding failed
  uint32_t decoding_failure_flag;

  // number of decoded elements ("bit errors")
  std::vector<ssize_t> decoded_num_differences;
  // decoded elements ("bit error positions")
  std::vector<uint64_t> decoded_differences;

  /**
   * @brief Constructor
   *
   * @param m            2^m -1 is the block length of BCH codes
   * @param t            error-correcting capacity of BCH codes
   * @param g            number of groups
   */
  PbsDecodingMessage(uint32_t m, uint32_t t, size_t g)
      : PbsMessage(PbsMessageType::DECODING),
        field_sz(m),
        capacity(t),
        num_groups(g),
        sizeof_each_d(utils::CeilLog2(capacity + 2)),
        decoding_failure_flag(utils::UIntXMax(sizeof_each_d)),
        decoded_num_differences(g),
        decoded_differences(g * capacity) {}
  ~PbsDecodingMessage() override = default;

  /**
   * @brief Set this message.
   *
   * @tparam WritableIterator                 writable iterator type
   * @tparam ReadOnlyIterator                 readonly iterator type
   * @param sketch_a_first, sketch_a_last     range of the sketches from one host (the merged result will
   *                                          be writen to)
   * @param sketch_b_first                    the beginning of the sketch range from another host
   */
  template<typename WritableIterator, typename ReadOnlyIterator>
  void setWith(WritableIterator sketch_a_first, WritableIterator sketch_a_last,
               ReadOnlyIterator sketch_b_first) {
    static_assert(
        std::is_same_v<
            typename std::iterator_traits<WritableIterator>::value_type,
            minisketch *>,
        "WritableIterator should have a value_type of minisketch*");
    static_assert(
        std::is_same_v<
            typename std::iterator_traits<ReadOnlyIterator>::value_type,
            minisketch *>,
        "ReadOnlyIterator should have a value_type of minisketch*");

    size_t g = 0, offset = 0;
    WritableIterator sketch_a_it = sketch_a_first;
    ReadOnlyIterator sketch_b_it = sketch_b_first;
    for (; sketch_a_it != sketch_a_last; ++sketch_a_it, ++sketch_b_it) {
      decoded_num_differences[g] =
          doDecoding(*sketch_a_it, *sketch_b_it, &decoded_differences[offset]);
      if (decoded_num_differences[g] > 0) offset += decoded_num_differences[g];
      ++g;
    }
    decoded_differences.resize(offset);
  }

  /**
   * @brief Parse from buffer.
   *
   * @param from            buffer to parse
   * @param msg_sz          buffer size (in bytes)
   * @return                sizes being parsed, returns -1 if failed
   */
  ssize_t parse(const uint8_t *from, size_t msg_sz) override {
    utils::BitReader reader(from);

    size_t count = 0;
    // read d part
    for (auto &decode_each_d : decoded_num_differences) {
      auto d = reader.Read<uint32_t>(sizeof_each_d);
      if (d == decoding_failure_flag)
        decode_each_d = -1;
      else {
        decode_each_d = static_cast<ssize_t>(d);
        count += d;
      }
    }
    // correct size
    decoded_differences.resize(count, 0);
    size_t total_bytes = serializedSize();
    if (total_bytes > msg_sz) return -1;

    // read difference parts
    for (auto &diff : decoded_differences) {
      diff = reader.Read<uint64_t>(field_sz);
    }
    return total_bytes;
  }

  /**
   * @brief  Write to buffer.
   *
   * @param to    buffer to write
   * @return      number of written bytes
   */
  ssize_t write(uint8_t *to) const override {
    utils::BitWriter writer(to);
    // write d part
    for (const auto &decode_each_d : decoded_num_differences) {
      auto d = decode_each_d < 0 ? decoding_failure_flag
                                 : static_cast<uint32_t>(decode_each_d);
      writer.Write<uint32_t>(d, sizeof_each_d);
    }

    // write difference parts
    for (const auto &decode_each_difference : decoded_differences) {
      writer.Write<uint64_t>(decode_each_difference, field_sz);
    }
    // tell writer I am finished
    writer.Flush();

    return serializedSize();
  }

  /**
   * @brief  Serialized size
   *
   * @return  serialized size (in bytes)
   */
  [[nodiscard]] ssize_t serializedSize() const override {
    size_t total_bits =
        sizeof_each_d * num_groups + field_sz * decoded_differences.size();
    return utils::Bits2Bytes(total_bits);
  }

 private:
  /**
   * @brief Do BCH decoding.
   *
   * @param sketch_a                 sketch for bitmap A
   * @param sketch_b                 sketch for bitmap B
   * @param difference_start         where to store the decoded elements
   * @return                         number of decoded elements, returns -1 when failed
   */
  inline ssize_t doDecoding(minisketch *sketch_a, const minisketch *sketch_b,
                            uint64_t *difference_start) const {
    minisketch_merge(sketch_a, sketch_b);
    return minisketch_decode(sketch_a, capacity, difference_start);
  }

};  // end PbsDecodingMessage
}  // end namespace libpbs

#endif  // _PBS_DECODING_MESSAGE_H_
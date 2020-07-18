/**
 * @file pbs_encoding_message.h
 * @author Long Gong <long.github@gmail.com>
 * @brief The PBS Encoding Message.
 * @version 0.1
 * @date 2020-07-06
 *
 * @copyright Copyright (c) 2020
 *
 */
#ifndef _PBS_ENCODING_MESSAGE_H
#define _PBS_ENCODING_MESSAGE_H

#include <minisketch.h>

#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

#include "bit_utils.h"
#include "pbs_message.h"

namespace libpbs {
/**
 * @brief The PbsEncodingMessage class.
 *
 * A PBS encoding message is a collection of Parity Bitmap Sketches
 * each of which uses BCH codes to doEncode_ necessary information
 * for the receiver to decode where his parity bitmaps (bitmaps
 * for his subsets/groups) are different from the sender's.
 *
 */
class PbsEncodingMessage : public PbsMessage {
 public:
  /**
   * @brief Constructor
   *
   *
   * @param m     field size
   * @param t     capacity
   * @param g     number of groups (g = 0 indicates that this message will be
   *              parsed from a buffer)
   */
  PbsEncodingMessage(uint32_t m, uint32_t t, uint32_t g)
      : PbsMessage(PbsMessageType::ENCODING),
        field_sz(m),
        n((1u << m) - 1u),
        capacity(t),
        num_groups(g) {
    create_sketches();
  }

  /// deconstructor (release memory)
  ~PbsEncodingMessage() override {
    for (auto sketch : sketches_) minisketch_destroy(sketch);
  }

  // field size
  uint32_t field_sz;
  // largest supported range (2^field_sz - 1)
  uint32_t n;
  // capacity, how many differences (different bit positions between the two
  // bitmaps) can this resulting correctly pinpoint
  uint32_t capacity;
  // number of subsets/groups
  uint32_t num_groups;

  /**
   * @brief Parse from buffer.
   *
   * @note To be able to parse, at least field_sz and capacity should be
   * available.
   *
   * @param from            buffer to parse
   * @param msg_sz          buffer size (in bytes)
   * @return                sizes being parsed, returns -1 if failed
   */
  ssize_t parse(const uint8_t *from, size_t msg_sz) override {
    // no enough information to parse the buffer
    if (field_sz == 0 || capacity == 0 || num_groups == 0) return -1;
    uint32_t sketch_sz = field_sz * capacity;
    uint32_t total_bytes = utils::Bits2Bytes(sketch_sz * num_groups);
    if (total_bytes > msg_sz) return -1;
    uint32_t sketch_bytes = sketch_sz / 8;
    uint32_t sketch_bits_remainder = sketch_sz % 8;
    if (sketch_bits_remainder == 0)
      parse_good(from, sketch_bytes);
    else
      parse_bad(from, sketch_bytes, sketch_bits_remainder);

    return total_bytes;
  }

  /**
   * @brief Get serialized size (in bytes)
   *
   * @return    serialized size
   */
  [[nodiscard]] ssize_t serializedSize() const override {
    // nothing to write
    if (field_sz == 0 || capacity == 0 || num_groups == 0) return -1;
    uint32_t total_bits = field_sz * capacity * num_groups;
    return utils::Bits2Bytes(total_bits);
  }

  /**
   * @brief Write this message to buffer
   *
   * NOTE THAT `to` must be allocated space, in other words, this
   * API will not take charge of allocating space for the buffer.
   *
   * @param to    buffer to write
   * @return      the number of bytes written, returns -1 if failed
   */
  ssize_t write(uint8_t *to) const override {
    uint32_t sketch_sz = field_sz * capacity;
    uint32_t sketch_bytes = sketch_sz / 8;
    uint32_t sketch_bits_remainder = sketch_sz % 8;
    size_t total_bytes = serializedSize();
    std::fill(to, to + total_bytes, 0);
    if (sketch_bits_remainder == 0)
      write_good(to, sketch_bytes);
    else
      write_bad(to, sketch_bytes, sketch_bits_remainder);

    return total_bytes;
  }

  /**
   * @brief Get all sketches
   *
   * @return   sketches
   */
  std::vector<minisketch *> &getSketches() { return sketches_; }

  /**
   * @brief Get the i-th sketch
   *
   * @param i    index of the sketch to get
   * @return     the i-th sketch, returns nullptr if not exist
   */
  minisketch *getSketch(size_t i) {
    if (i >= num_groups) return nullptr;
    return sketches_[i];
  }

  /**
   * @brief Get all sketches (readonly)
   *
   * @return   sketches
   */
  [[nodiscard]] const std::vector<minisketch *> &getSketches() const {
    return sketches_;
  }

  /**
   * @brief Get the i-th sketch (read-only)
   *
   * @param i    index of the sketch to get
   * @return     the i-th sketch, returns nullptr if not exist
   */
  [[nodiscard]] const minisketch *getSketch(size_t i) const {
    if (i >= num_groups) return nullptr;
    return sketches_[i];
  }

 private:
  /**
   * @brief Write for the good cases
   *
   * Here, good cases mean the cases where the size of the each sketch (in bits)
   * is divided by 8.
   *
   * @param to               buffer to write
   * @param sketch_bytes     bytes per sketch
   */
  void write_good(uint8_t *to, uint32_t sketch_bytes) const {
    for (uint32_t i = 0; i < num_groups; ++i)
      minisketch_serialize(sketches_[i], to + i * sketch_bytes);
  }

  /**
   * @brief  Write for the bad cases.
   *
   * Here, bad cases mean the cases where the size of the each sketch (in bits)
   * is not divided by 8. For example, if each sketch takes 49 bits, then
   * sketch_bytes is 6 and sketch_bits_remainder is 1.
   *
   * @param to                          buffer to write
   * @param sketch_bytes                bytes per sketch
   * @param sketch_bits_remainder       bits for each sketch
   */
  void write_bad(uint8_t *to, uint32_t sketch_bytes,
                 uint32_t sketch_bits_remainder) const {
    auto *buf = (uint8_t *)malloc(sketch_bytes + 1);
    uint32_t remainder_pos_start = sketch_bytes * num_groups;
    utils::BitWriter writer(to + remainder_pos_start);
    for (uint32_t i = 0; i < num_groups; ++i) {
      memset(buf, 0, sketch_bytes + 1);
      minisketch_serialize(sketches_[i], buf);
      memcpy(to + i * sketch_bytes, buf, sketch_bytes);
      writer.Write<uint8_t>(buf[sketch_bytes], sketch_bits_remainder);
    }
    // tell writer I am finished
    writer.Flush();
    free(buf);
  }

  /**
   * @brief  Parse bad cases
   *
   * @param from                          buffer to parse
   * @param sketch_bytes                  bytes per sketch
   * @param sketch_bits_remainder         bits for each sketch
   */
  void parse_bad(const uint8_t *from, uint32_t sketch_bytes,
                 uint8_t sketch_bits_remainder) {
    auto *buf = (uint8_t *)malloc(sketch_bytes + 1);
    uint32_t remainder_pos_start = sketch_bytes * num_groups;
    utils::BitReader reader(from + remainder_pos_start);
    for (uint32_t i = 0; i < num_groups; ++i) {
      memcpy(buf, from + i * sketch_bytes, sketch_bytes);
      buf[sketch_bytes] = reader.Read<uint8_t>(sketch_bits_remainder);
      minisketch_deserialize(sketches_[i], buf);
    }
    free(buf);
  }

  /**
   * @brief Parse good cases
   *
   * @param from             buffer to parse
   * @param sketch_bytes     bytes per sketch
   */
  void parse_good(const uint8_t *from, uint32_t sketch_bytes) {
    sketches_.clear();
    auto *buf = (uint8_t *)malloc(sketch_bytes + 1);
    for (uint32_t i = 0; i < num_groups; ++i) {
      minisketch *parsed_sketch = minisketch_create(field_sz, 0, capacity);
      memcpy(buf, from + i * sketch_bytes, sketch_bytes);
      minisketch_deserialize(parsed_sketch, buf);
      sketches_.push_back(parsed_sketch);
    }
    free(buf);
  }

  /**
   * @brief Create sketches.
   *
   */
  void create_sketches() {
    for (size_t i = 0; i < num_groups; ++i) {
      sketches_.push_back(minisketch_create(field_sz, 0, capacity));
    }
  }
  std::vector<minisketch *> sketches_;
};

}  // namespace libpbs

#endif  //_PBS_ENCODING_MESSAGE_H

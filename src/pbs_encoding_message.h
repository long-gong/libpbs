#ifndef _PBS_ENCODING_MESSAGE_H
#define _PBS_ENCODING_MESSAGE_H

#include <minisketch.h>

#include <memory>
#include <vector>
#include <cstring>
#include <cstdlib>

#include "pbs_message.h"
#include "bit_utils.h"

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
   * parsed from a buffer)
   */
  PbsEncodingMessage(uint32_t m, uint32_t t, uint32_t g)
      : PbsMessage(PbsMessageType::ENCODING),
        field_sz(m),
        n((1u << m) - 1u),
        capacity(t),
        num_groups(g){
    create_sketches();
  }

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

  /// To be able to parse, at least field_sz and capacity should be available.
  ssize_t parse(const uint8_t *from, size_t msg_sz) override {
    // no enough information to parse the buffer
    if (field_sz == 0 || capacity == 0 || num_groups == 0) return -1;
    uint32_t sketch_sz = field_sz * capacity;
    uint32_t total_bytes = (sketch_sz * num_groups + 7) / 8;
    if (total_bytes > msg_sz) return -1;
    uint32_t sketch_bytes = sketch_sz / 8;
    uint32_t sketch_bits_remainder = sketch_sz % 8;
    if (sketch_bits_remainder == 0)
      parse_good(from, sketch_bytes);
    else
      parse_bad(from, sketch_bytes, sketch_bits_remainder);

    return total_bytes;
  }

  ssize_t serializedSize() const override {
    // nothing to write
    if (field_sz == 0 || capacity == 0 || num_groups == 0) return -1;
    uint32_t total_bytes = (field_sz * capacity * num_groups + 7 / 8);
    return total_bytes;
  }

  /// `to` must be allocated space 
  ssize_t write(uint8_t *to) const override {
    uint32_t sketch_sz = field_sz * capacity;
    uint32_t sketch_bytes = sketch_sz / 8;
    uint32_t sketch_bits_remainder = sketch_sz % 8;
    size_t total_bytes = serializedSize();
    memset(to, 0, total_bytes);
    if (sketch_bits_remainder == 0)
      write_good(to, sketch_bytes);
    else
      write_bad(to, sketch_bytes, sketch_bits_remainder);

    return total_bytes;
  }

  std::vector<minisketch *> &getSketches() {
    return sketches_;
  }

  minisketch *getSketch(size_t i) {
    if (i >= num_groups) return nullptr;
    return sketches_[i];
  }

  [[nodiscard]] const std::vector<minisketch *> &getSketches() const { return sketches_; }

  [[nodiscard]] const minisketch *getSketch(size_t i) const {
    if (i >= num_groups) return nullptr;
    return sketches_[i];
  }

 private:

  void write_good(uint8_t *to, uint32_t sketch_bytes) const {
    for (uint32_t i = 0; i < num_groups; ++i)
      minisketch_serialize(sketches_[i], to + i * sketch_bytes);
  }

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
    free(buf);
  }

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

  void create_sketches() {
    for (size_t i = 0;i < num_groups;++ i) {
      sketches_.push_back(minisketch_create(field_sz, 0, capacity));
    }
  }
  std::vector<minisketch *> sketches_;
};

}  // namespace libpbs

#endif  //_PBS_ENCODING_MESSAGE_H

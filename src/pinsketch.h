#ifndef PINSKETCH_H_
#define PINSKETCH_H_

#include <minisketch.h>

class PinSketch {
 public:
  PinSketch() : sketch_(nullptr) {}
  PinSketch(size_t m, size_t t) : sketch_(minisketch_create(m, 0, t)) {
  }

  std::string name() const {
    return "PinSketch";
  }

  size_t bits() const {
    assert(sketch_ != nullptr);
    return minisketch_bits(sketch_);
  }

  size_t capacity() const {
    assert(sketch_ != nullptr);
    return minisketch_capacity(sketch_);
  }

  template<typename Iterator>
  void encode(Iterator first, Iterator last) {
    assert(sketch_ != nullptr);
    for (auto it = first; it != last; ++it)
      minisketch_add_uint64(sketch_, static_cast<uint64_t>(*it));
  }

  template<typename Iterator>
  std::string encode_and_serialize(Iterator first, Iterator last) {
    encode(first, last);
    return serialize();
  }

  template<typename Iterator>
  void encode_key_value_pairs(Iterator first, Iterator last) {
    assert(sketch_ != nullptr);
    for (auto it = first; it != last; ++it)
      minisketch_add_uint64(sketch_, static_cast<uint64_t>(it->first));
  }

  template<typename Iterator>
  std::string encode_and_serialize_key_value_pairs(Iterator first, Iterator last) {
    assert(sketch_ != nullptr);
    for (auto it = first; it != last; ++it)
      minisketch_add_uint64(sketch_, static_cast<uint64_t>(it->first));
    return serialize();
  }

  bool decode(unsigned char *other,
              std::vector<uint64_t> &differences) const {
    assert(sketch_ != nullptr);
    minisketch *sketch_other = minisketch_create(bits(), 0, capacity());
    minisketch_deserialize(sketch_other, other); //
    minisketch_merge(sketch_other, sketch_);

    differences.resize(minisketch_capacity(sketch_), 0);
    ssize_t num_differences = minisketch_decode(sketch_other, capacity(), &differences[0]);
    minisketch_destroy(sketch_other);
    if (num_differences == -1) return false;
    differences.resize(num_differences);
    return true;
  }

  ~PinSketch() {
    if (sketch_ != nullptr) minisketch_destroy(sketch_);
  }

 private:
  std::string serialize() const {
    assert(sketch_!= nullptr);
    size_t sersize = minisketch_serialized_size(sketch_);
    std::string buffer(sersize, ' ');
    minisketch_serialize(sketch_, (unsigned char *) &buffer[0]);
    return buffer;
  }
  minisketch *sketch_;
};

#endif //PINSKETCH_H_

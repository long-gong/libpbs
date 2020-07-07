// https://github.com/sipa/minisketch/blob/master/src/int_utils.h
#ifndef _BIT_UTILS_H_
#define _BIT_UTILS_H_

#include <cinttypes>

namespace libpbs {
namespace utils {
namespace constants {
constexpr std::size_t BITS_IN_ONE_BYTE = 8;
}
inline uint32_t UIntXMax(uint32_t bits) { return (1u << (bits - 1u)) - 1u; }
inline std::size_t Bits2Bytes(std::size_t bits) {
  return (bits + constants::BITS_IN_ONE_BYTE - 1) / constants::BITS_IN_ONE_BYTE;
}

class BitWriter {
  unsigned char state = 0;
  int offset = 0;
  unsigned char *out;

 public:
  BitWriter(unsigned char *output) : out(output) {}

  template <typename UInteger>
  inline void Write(UInteger val, int bits) {
    if (bits + offset >= 8) {
      state |= ((val & ((UInteger(1) << (8 - offset)) - 1)) << offset);
      *(out++) = state;
      val >>= (8 - offset);
      bits -= 8 - offset;
      offset = 0;
      state = 0;
    }
    while (bits >= 8) {
      *(out++) = val & 255;
      val >>= 8;
      bits -= 8;
    }
    state |= ((val & ((UInteger(1) << bits) - 1)) << offset);
    offset += bits;
  }

  inline void Flush() {
    if (offset) {
      *(out++) = state;
      state = 0;
      offset = 0;
    }
  }
};

class BitReader {
  unsigned char state = 0;
  int offset = 0;
  const unsigned char *in;

 public:
  BitReader(const unsigned char *input) : in(input) {}

  template <typename UInteger>
  inline UInteger Read(int bits) {
    if (offset >= bits) {
      UInteger ret = state & ((1 << bits) - 1);
      state >>= bits;
      offset -= bits;
      return ret;
    }
    UInteger val = state;
    int out = offset;
    while (out + 8 <= bits) {
      val |= ((UInteger(*(in++))) << out);
      out += 8;
    }
    if (out < bits) {
      unsigned char c = *(in++);
      val |= (c & ((UInteger(1) << (bits - out)) - 1)) << out;
      state = c >> (bits - out);
      offset = 8 - (bits - out);
    } else {
      state = 0;
      offset = 0;
    }
    return val;
  }
};
}  // namespace utils
}  // namespace libpbs

#endif  // BIT_UTILS_H_

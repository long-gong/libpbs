// https://github.com/sipa/minisketch/blob/master/src/int_utils.h
#ifndef _BIT_UTILS_H_
#define _BIT_UTILS_H_

#include <cinttypes>
#include <type_traits>
#include <cmath>
#include <tsl/ordered_set.h>

namespace libpbs::utils {
namespace constants {
constexpr std::size_t BITS_IN_ONE_BYTE = 8;
}

inline uint32_t UIntXMax(uint32_t bits) { return (1u << bits) - 1u; }
inline std::size_t Bits2Bytes(std::size_t bits) {
  return (bits + constants::BITS_IN_ONE_BYTE - 1) / constants::BITS_IN_ONE_BYTE;
}

template<typename T>
inline uint64_t CeilLog2(T data) {
  static_assert(std::is_scalar_v<T>, "T must be a scalar type");
  return static_cast<uint64_t>(std::ceil(std::log2(data)));
}

template <typename T>
inline std::vector<T> setDifference(const tsl::ordered_set<T>& sa, const tsl::ordered_set<T>& sb) {
  std::vector<T> result;
  for (const auto& item: sa) {
    if (!sb.contains(item)) result.push_back(item);
  }
  for (const auto& item: sb) {
    if (!sa.contains(item)) result.push_back(item);
  }
  return result;
}

template <typename T>
inline void setDifference(std::vector<T>& sa, const std::vector<T>& sb)  {
    tsl::ordered_set<T> s1(sa.begin(),sa.end());
    tsl::ordered_set<T> s2(sb.cbegin(), sb.cend());
    sa = setDifference(s1, s2);
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
}  // namespace libpbs

#endif  // BIT_UTILS_H_

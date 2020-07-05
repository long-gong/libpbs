#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include <cinttypes>
#include <string>
#include <cmath>

constexpr unsigned DEFAULT_SKETCHES_ = 128;
constexpr unsigned DEFAULT_SEED = 142857;
constexpr double INFLATION_RATIO = 1.38;
constexpr unsigned BITS_IN_ONE_BYTE = 8;
constexpr unsigned RETIRES = 5;

using Key = int32_t;
using Value = std::string;

/// make sure the estimate is at least as large as the
/// ground truth with at least a probability of 0.99
#define ESTIMATE_SM99(estimate) static_cast<size_t>(std::ceil((INFLATION_RATIO * estimate)))

#endif //CONSTANTS_H_

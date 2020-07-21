// Generating random data

#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <random>
#include <stdexcept>
#include <vector>

// https://raw.githubusercontent.com/efficient/cuckoofilter/master/benchmarks/random.h
::std::vector<int> GenerateRandomSet32(::std::size_t count) {
  ::std::vector<int> result(size_t(count * 1.1));
  ::std::random_device random;
  // To generate random keys to lookup, this uses ::std::random_device which is
  // slower but stronger than some other pseudo-random alternatives. The reason
  // is that some of these alternatives (like libstdc++'s ::std::default_random,
  // which is a linear congruential generator) behave non-randomly under some
  // hash families like Dietzfelbinger's multiply-shift.
  auto genrand = [&random]() {
    return random() ;
  };
  ::std::generate(result.begin(), result.end(), ::std::ref(genrand));
  std::sort(result.begin(), result.end());
  auto it = std::unique(result.begin(), result.end());
  result.resize(std::distance(result.begin(), it));

  std::mt19937_64 g(random());
  std::shuffle(result.begin(), result.end(), g);
  result.resize(count);
  return result;
}


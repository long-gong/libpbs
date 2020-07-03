#ifndef CACHE_HELPER_H_
#define CACHE_HELPER_H_

#include <fmt/format.h>
#include <wyhash.h>

#include <array>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>
#include <string>

#include "eigen_boost_serialization.hpp"

#if __cplusplus >= 201703L
/* Now remove the trow */
#define throw( \
    ...)  //  error: ISO C++1z does not allow dynamic exception specifications
#include <stlcache/stlcache.hpp>
#undef throw /* reset */
#else
#include <stlcache/stlcache.hpp>
#endif

#define YELLOW "\033[33m" /* Yellow */
#define DEFAULT_CACHE_DIR "../cache/"


namespace {
const char* cache_file_pat_ = "multiple_round_m2d_{}_{}_{}_{}.eigen3";
constexpr unsigned hash_seed_ = 172854;
constexpr unsigned max_cache_ = 1024;
struct hashfn {
  uint64_t operator()(const std::array<size_t, 4>& key) const {
    return wyhash(&key[0], sizeof(size_t) * key.size(), hash_seed_, _wyp);
  }
};
}  // namespace

namespace pbsutils {
using key_t = std::array<size_t, 4>;
using value_t = Eigen::MatrixXd;
using cache_t = stlcache::cache<key_t, value_t, stlcache::policy_lru>;
inline cache_t& get_memcache() {
  static cache_t _my_memcache(max_cache_);
  return _my_memcache;
}
inline void memcache_write(const key_t& key, const value_t& val) {
  get_memcache().insert(key, val);
}
inline bool memcache_check(const key_t& key) {
  return get_memcache().check(key);
}
inline value_t memcache_fetch(const key_t& key) {
  return get_memcache().fetch(key);
}
inline std::string get_cache_filename(const key_t& key) {
  return std::string(DEFAULT_CACHE_DIR) +
         fmt::format(cache_file_pat_, key[0], key[1], key[2], key[3]);
}
inline void save_cache(const key_t& key, const value_t& val) {
  auto fn = get_cache_filename(key);
  if (boost::filesystem::remove(fn)) {
    std::cout << YELLOW << "Overwriting existing cache file\n";
  }
  std::ofstream fout(fn);
  boost::archive::binary_oarchive ar(fout);
  ar& val;
}
inline bool load_cache(const key_t& key, value_t& val) {
  auto fn = get_cache_filename(key);
  std::ifstream fin(fn);
  if (!fin.is_open()) return false;
  boost::archive::binary_iarchive ar(fin);
  ar& val;
  return true;
}
}  // namespace pbsutils

#endif  // CACHE_HELPER_H_
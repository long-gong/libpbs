#ifndef CACHE_HELPER_H_
#define CACHE_HELPER_H_

#include <fmt/format.h>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem.hpp>

#include <array>
#include <fstream>
#include <iostream>
#include <string>

#include "eigen_boost_serialization.hpp"

// TODO: change to boost::cache
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <stlcache/stlcache.hpp>
#pragma GCC diagnostic pop

#define YELLOW "\033[33m" /* Yellow */
#define DEFAULT_CACHE_DIR "../cache/"

/// constants and functions that will not exposed
namespace {
const char* cache_file_pat_ = "multiple_round_m2d_{}_{}_{}_{}.eigen3";
constexpr unsigned max_cache_ = 1024;
inline void make_sure_cache_dir_exists() {
  boost::filesystem::path dir(DEFAULT_CACHE_DIR);
  if (!boost::filesystem::exists(dir)) boost::filesystem::create_directories(dir);
}
}  // namespace

/// utility functions to be exposed
namespace pbsutils {
using key_t = std::array<size_t, 4>;
using value_t = Eigen::MatrixXd;
using cache_t = stlcache::cache<key_t, value_t, stlcache::policy_lru>;

/**
 * @brief Get memory cache
 *
 * @return  memory cache
 */
inline cache_t& get_memcache() {
  static cache_t _my_memcache(max_cache_);
  return _my_memcache;
}

/**
 * @brief Write memory cache
 *
 * @param key       key value associated with the element to cache
 * @param val       element to cache
 */
inline void memcache_write(const key_t& key, const value_t& val) {
  get_memcache().insert(key, val);
}

/**
 * @brief  Check memory cache
 *
 * @param key      key value of the element to check for
 * @return         whether an element associated with this key exists
 */
inline bool memcache_check(const key_t& key) {
  return get_memcache().check(key);
}

/**
 * @brief Fetch memory cache
 *
 * Note that please make sure the element associated with this key do exist
 * (using memcache_check) before calling this function.
 *
 * @param key   key value of the element to fetch for
 * @return      the element associated with this key
 */
inline value_t memcache_fetch(const key_t& key) {
  return get_memcache().fetch(key);
}

/**
 * @brief Release the memory allocated for the memory cache
 *
 */
inline void memcache_clear() {
  get_memcache().clear();
}
/**
 * @brief Get the filename for disk cache
 *
 * @param key   key value of the element of which the filename to get
 * @return      filename for disk cache for the element associated with this key
 */
inline std::string get_cache_filename(const key_t& key) {
  make_sure_cache_dir_exists();
  return std::string(DEFAULT_CACHE_DIR) +
      fmt::format(cache_file_pat_, key[0], key[1], key[2], key[3]);
}

/**
 * @brief Save content to disk cache (file on disk)
 *
 * @param key    key value associated with the element to cache
 * @param val    value to cache
 */
inline void save_cache(const key_t& key, const value_t& val) {
  auto fn = get_cache_filename(key);
  if (boost::filesystem::remove(fn)) {
    std::cout << YELLOW << "Overwriting existing cache file\n";
  }
  std::ofstream fout(fn);
  boost::archive::binary_oarchive ar(fout);
  ar& val;
}

/**
 * @brief Load cache from disk
 *
 * @param key  key value associated with the element to load
 * @param val  value to load
 * @return     whether an element associated with this key exists
 */
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
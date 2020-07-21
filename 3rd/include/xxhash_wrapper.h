#ifndef XXHASH_WRAPPER_H_
#define XXHASH_WRAPPER_H_

// change to the latest version
//#include <xxhash.h>
#include <xxh3.h>
#include <chrono>

struct XXHash {
  explicit XXHash(
      unsigned rs = std::chrono::system_clock::now().time_since_epoch().count())
      : random_seed_(rs) {}
  uint32_t operator()(uint32_t key) const { return hash(key); }

 private:
  [[nodiscard]] uint32_t hash(const uint32_t key) const {
    return XXH32(&key, sizeof(key), random_seed_);
  }

  unsigned random_seed_;
};

struct XXHash64 {
  explicit XXHash64(
      unsigned rs = std::chrono::system_clock::now().time_since_epoch().count())
      : random_seed_(rs) {}
  uint64_t operator()(uint64_t key) const { return hash(key); }

 private:
  [[nodiscard]] uint64_t hash(const uint64_t key) const {
    return XXH64(&key, sizeof(key), random_seed_);
  }

  unsigned random_seed_;
};

#endif // XXHASH_WRAPPER_H_
#include <gtest/gtest.h>

#include "cache_helper.h"

TEST(CacheHelperTest, Memcache) {
    std::array<size_t, 4> key{1,2,3,4};

    auto exists = pbsutils::memcache_check(key);
    EXPECT_FALSE(exists);

    Eigen::MatrixXd mat(2,2);
    mat << 1, 2, 3, 4;
    pbsutils::memcache_write(key, mat);

    auto mat_from_cache = pbsutils::memcache_fetch(key);
    EXPECT_EQ(mat_from_cache, mat);

    std::array<size_t, 4> n_key{2,3,3,3};
    EXPECT_THROW(
        pbsutils::memcache_fetch(n_key), stlcache::exception_invalid_key
    );
}


TEST(CacheHelperTest, FileCache) {
    std::array<size_t, 4> key{1,2,3,4};
    Eigen::MatrixXd mat(2,2);
    auto exists = pbsutils::load_cache(key, mat);
    EXPECT_FALSE(exists);

    mat << 1, 2, 3, 4;

    pbsutils::save_cache(key, mat);

    Eigen::MatrixXd mat_from_cache;
    exists = pbsutils::load_cache(key, mat_from_cache);

    EXPECT_EQ(mat_from_cache, mat);
    EXPECT_TRUE(exists);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
  return 0;
}
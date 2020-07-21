#include <bloom/bloom_filter.h>
#include <gtest/gtest.h>

TEST(BloomTest, Serialization) {
  bloom_parameters parameters;
  // How many elements roughly do we expect to insert?
  parameters.projected_element_count = 1000;
  // Maximum tolerable false positive probability? (0,1)
  parameters.false_positive_probability = 0.0001;  // 1 in 10000
  // Simple randomizer (optional)
  parameters.random_seed = 0xA5A5A5A5;
  parameters.compute_optimal_parameters();
  bloom_filter filter(parameters);

  for (std::size_t i = 0; i < parameters.projected_element_count; ++i) {
    filter.insert(i);
  }
  auto buffer = filter.table();

  bloom_filter another_filter(parameters);
  another_filter.set(buffer, buffer + filter.size() / 8);
  for (std::size_t i = 0; i < parameters.projected_element_count; ++i) {
    EXPECT_TRUE(another_filter.contains(i));
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

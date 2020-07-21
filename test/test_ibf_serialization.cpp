#include <gtest/gtest.h>
#include <iblt/iblt.h>

#include <algorithm>

TEST(IbfTests, Serialization) {
  const int VAL_SIZE = 1;  // bytes
  const std::vector<uint8_t> VAL{0};
  float HEDGE = 10.0;
  size_t num_inserted = 100;
  size_t num_hashes = 3;
  IBLT iblt(num_inserted, VAL_SIZE, HEDGE, num_hashes);

  for (size_t i = 1; i <= num_inserted; ++i) iblt.insert(i, VAL);
  auto buffer = iblt.data();

  IBLT iblt2(num_inserted, VAL_SIZE, HEDGE, num_hashes);
  using CellIterator = decltype(buffer.cbegin());
  iblt2.set(
      buffer.cbegin(), buffer.cend(), [](CellIterator it) { return it->count; },
      [](CellIterator it) { return it->keySum; },
      [](CellIterator it) { return it->keyCheck; });

  std::set<std::pair<uint64_t, std::vector<uint8_t>>> pos, neg;
  bool succeed = iblt2.listEntries(pos, neg);

  EXPECT_TRUE(succeed);
  EXPECT_TRUE(neg.empty());
  for (size_t i = 1; i <= num_inserted; ++i) {
    EXPECT_TRUE(pos.count({i, VAL}));
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

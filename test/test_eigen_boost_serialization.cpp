#include <gtest/gtest.h>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <fstream>

#include "eigen_boost_serialization.hpp"

namespace {

class EigenBoostSerializationTest : public ::testing::Test {
 protected:
  // Remember that SetUp() is run immediately before a test starts.
  // This is a good place to record the start time.
  void SetUp() override {
    mat_test_ = Eigen::MatrixXd{2, 2};
    mat_test_ << 1, 2, 3, 4;

    fn_test_ = "test.eigen3";
  }

  // TearDown() is invoked immediately after a test finishes.  Here we
  // check if the test was too slow.
  void TearDown() override {
    // do nothing
  }
  Eigen::MatrixXd mat_test_;
  std::string fn_test_;
};

// Now we can write tests in the IntegerFunctionTest test case.

// Tests serialization & deserialization
TEST_F(EigenBoostSerializationTest, SerializationAndDeserialization) {
  // Serialization
  {
    std::ofstream fout(fn_test_);
    boost::archive::binary_oarchive ar(fout);
    ASSERT_NO_THROW(ar & mat_test_);
  }
  // Deserialization
  {
    std::ifstream fin(fn_test_);
    boost::archive::binary_iarchive ar(fin);
    Eigen::MatrixXd mat;
    ar &mat;
    EXPECT_EQ(mat, mat_test_);
  }
}
}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
  return 0;
}
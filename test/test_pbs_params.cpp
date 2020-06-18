#include "pbs_params.h"
#include <gtest/gtest.h>

TEST(PbsParamsTest, m3d) {
  size_t m =6, n = 8;
  auto mat = pbsutils::PbsParam::computeProbabilityMatrix3D(m, n);
  double abs_err = 1e-6;
  EXPECT_NEAR(mat[1](7, 2), 1.00000000, abs_err);
  EXPECT_NEAR(mat[2](6, 3), 0.87500000, abs_err);
  EXPECT_NEAR(mat[2](7, 1), 0.12500000, abs_err);
  EXPECT_NEAR(mat[3](5, 4), 0.65625000, abs_err);
  EXPECT_NEAR(mat[3](6, 2), 0.32812500, abs_err);
  EXPECT_NEAR(mat[3](7, 1), 0.01562500, abs_err);
  EXPECT_NEAR(mat[3](5, 4), 0.65625000, abs_err);
  EXPECT_NEAR(mat[3](6, 2), 0.32812500, abs_err);
  EXPECT_NEAR(mat[3](7, 1), 0.01562500, abs_err);
  EXPECT_NEAR(mat[4](4, 5), 0.41015625, abs_err);
  EXPECT_NEAR(mat[4](5, 3), 0.49218750, abs_err);
  EXPECT_NEAR(mat[4](6, 1), 0.04101562, abs_err);
  EXPECT_NEAR(mat[4](6, 2), 0.05468750, abs_err);
  EXPECT_NEAR(mat[4](7, 1), 0.00195312, abs_err);
  EXPECT_NEAR(mat[5](3, 6), 0.20507812, abs_err);
  EXPECT_NEAR(mat[5](4, 4), 0.51269531, abs_err);
  EXPECT_NEAR(mat[5](5, 2), 0.15380859, abs_err);
  EXPECT_NEAR(mat[5](5, 3), 0.10253906, abs_err);
  EXPECT_NEAR(mat[5](6, 1), 0.01708984, abs_err);
  EXPECT_NEAR(mat[5](6, 2), 0.00854492, abs_err);
  EXPECT_NEAR(mat[5](7, 1), 0.00024414, abs_err);
}

TEST(PbsParamsTest, m2d) {
  size_t m = 5, n = 128, t= 5;
  auto mat = pbsutils::PbsParam::computeTransitionProbabilityMatrix(m, n, t);
  double abs_err = 1e-6;
  EXPECT_NEAR(mat(1, 1), 1.00000000, abs_err);
  EXPECT_NEAR(mat(2, 1), 0.99218750, abs_err);
  EXPECT_NEAR(mat(2, 3), 0.00781250, abs_err);
  EXPECT_NEAR(mat(3, 1), 0.97668457, abs_err);
  EXPECT_NEAR(mat(3, 3), 0.02325439, abs_err);
  EXPECT_NEAR(mat(3, 4), 0.00006104, abs_err);
  EXPECT_NEAR(mat(4, 1), 0.95379353, abs_err);
  EXPECT_NEAR(mat(4, 3), 0.04578209, abs_err);
  EXPECT_NEAR(mat(4, 4), 0.00024223, abs_err);
  EXPECT_NEAR(mat(4, 5), 0.00018215, abs_err);
  EXPECT_NEAR(mat(5, 1), 0.92398748, abs_err);
  EXPECT_NEAR(mat(5, 3), 0.07451512, abs_err);
  EXPECT_NEAR(mat(5, 4), 0.00059612, abs_err);
  EXPECT_NEAR(mat(5, 5), 0.00089655, abs_err);
  EXPECT_NEAR(mat(5, 6), 0.00000473, abs_err);
}

TEST(PbsParamsTest, mrm2d) {
  size_t m = 5, n = 128, t= 5, r = 2;
  auto mat = pbsutils::PbsParam::computeMultiRoundProbabilityMatrix(m, n, t, r);
  double abs_err = 1e-6;
  EXPECT_NEAR(mat(2, 1), 0.00781250, abs_err);
  EXPECT_NEAR(mat(2, 2), 0.00006104, abs_err);
  EXPECT_NEAR(mat(3, 1), 0.02331543, abs_err);
  EXPECT_NEAR(mat(3, 2), 0.00018310, abs_err);
  EXPECT_NEAR(mat(4, 1), 0.04620647, abs_err);
  EXPECT_NEAR(mat(4, 2), 0.00037174, abs_err);
  EXPECT_NEAR(mat(5, 1), 0.07601252, abs_err);
  EXPECT_NEAR(mat(5, 2), 0.00063783, abs_err);
}

TEST(PbsParamsTest, failprobub) {
  size_t m = 5, n = 128, t= 5, r = 2;
  auto mat = pbsutils::PbsParam::computeMultiRoundProbabilityMatrix(m, n, t, r);
  double abs_err = 1e-6;
  EXPECT_NEAR(pbsutils::PbsParam::computeFailureProbabilityBound(mat, 3, 100, 5, 2), 0.00036984, abs_err);
  EXPECT_NEAR(pbsutils::PbsParam::computeFailureProbabilityBound(mat, 2, 100, 5, 2), 0.00024535, abs_err);
  EXPECT_NEAR(pbsutils::PbsParam::computeFailureProbabilityBound(mat, 4, 100, 5, 2), 0.00049555, abs_err);
  EXPECT_NEAR(pbsutils::PbsParam::computeFailureProbabilityBound(mat, 5, 128, 5, 2), 0.00061981, abs_err);
}

TEST(PbsParamsTest, failprobub_final) {
  size_t d = 20, n = 512, t = 8, r = 2, c = 3;
  double delta = 5.0;
  double abs_err = 1e-6;
  EXPECT_NEAR(pbsutils::PbsParam::failureProbabilityUB(d, delta, n, r, t, c), 0.06558745, abs_err);
}

TEST(PbsParamsTest, best_param) {
  size_t d = 20, n = 512, t = 8, r = 2, c = 3;
  double delta = 5.0, obj_prob = 0.99;
  double abs_err = 1e-6;
  pbsutils::BestBchParam param{};
  auto ub = pbsutils::PbsParam::bestBchParam(d, delta, r, c, obj_prob, param);

  EXPECT_EQ(8, param.n);
  EXPECT_EQ(11, param.t);
  EXPECT_NEAR(0.009357799909271, ub, abs_err);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


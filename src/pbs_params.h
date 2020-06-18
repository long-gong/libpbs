#ifndef PBS_PARAMS_H_
#define PBS_PARAMS_H_

#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/Sparse>
#include <vector>
#include <stats.hpp>

namespace pbsutils {
constexpr size_t MAX_BALLS = 200;
constexpr size_t N_MIN = 6;
constexpr size_t N_MAX = 14;

using SpMat = Eigen::SparseMatrix<double>; // declares a column-major sparse
// matrix type of double
using Mat = Eigen::MatrixXd;

struct BestBchParam {
  size_t n;
  size_t t;
};

class PbsParam {
 public:
  static double bestBchParam(size_t d, double delta, size_t r, size_t c, double targetProb, BestBchParam &bch_param) {
    double best_cost = std::numeric_limits<double>::max(), cost = 0;
    size_t n = 1, t = 1;
    double failure_prob_ub = -1.0;

    for (size_t i = N_MIN; i <= N_MAX; ++i) {
      auto t_min = i;
      auto t_max = std::min(MAX_BALLS, std::min((1lu << i) - 2lu, size_t(std::ceil(5 * delta))));
      auto j = (1lu << i) - 1;
      auto p_min = 1 - failureProbabilityUB(d, delta, j, r, t_min, c);
      auto p_max = 1 - failureProbabilityUB(d, delta, j, r, t_max, c);

      if (p_min >= targetProb) {
        cost = static_cast<double>(t_min) * i;
        if (cost < best_cost) {
          best_cost = cost;
          n = i;
          t = t_min;
          failure_prob_ub = 1 - p_min;
        }
      } else if (p_max >= targetProb) {
        size_t t_mid = 0;
        while (t_max - t_min > 1u) {
          t_mid = t_min + (t_max - t_min) / 2;
          auto p = 1 - failureProbabilityUB(d, delta, j, r, t_mid, c);
          if (p >= targetProb) t_max = t_mid;
          else t_min = t_mid;
        }
        auto p = 1 - failureProbabilityUB(d, delta, j, r, t_min, c);
        size_t t_tmp = 0;
        if (p >= targetProb) t_tmp = t_min;
        else {
          t_tmp = t_max;
          p = 1 - failureProbabilityUB(d, delta, j, r, t_max, c);
        }
        cost = static_cast<double>(t_tmp) * i;
        if (cost < best_cost) {
          best_cost = cost;
          n = i;
          t = t_tmp;
          failure_prob_ub = 1 - p;
        }
      }
    }

    bch_param.n = n;
    bch_param.t = t;
    return failure_prob_ub;
  }

  static double computeFailureProbabilityBound(const Mat &mr_m2d, size_t m, size_t n, size_t t, size_t r) {
    double prob_fail_one_group = 0;
    double prob_tail = 1.0;
    for (size_t i = 0; i < t; ++i) {
      double p = stats::dbinom(i, m, 1.0 / n);
      prob_fail_one_group += p * mr_m2d(i + 1, r);
      prob_tail -= p;
    }
    prob_fail_one_group += prob_tail;
    return 2.0 * (1.0 - std::pow(1.0 - prob_fail_one_group, n));
  }

  static double failureProbabilityUB(size_t d, double delta, size_t n, size_t r, size_t t, size_t c) {
    auto g = d / delta;
    size_t m = std::min(MAX_BALLS, n - 1);
    auto mr_md = computeMultiRoundProbabilityMatrix(m, n, t, r);
    double prob_fail_one_group = 0;
    double p = 0, prob_tail = 1.0;

    for (size_t i = 0; i < t; ++i) {
      p = stats::dbinom(i, d, 1.0 / g);
      prob_fail_one_group += p * mr_md(i + 1, r);
      prob_tail -= p;
    }

    for (size_t i = t; i < m; ++i) {
      p = stats::dbinom(i, d, 1.0 / g);
      prob_fail_one_group += p * computeFailureProbabilityBound(mr_md, i, c, t, r - 1);
      prob_tail -= p;
    }

    prob_fail_one_group += prob_tail;
    return 2.0 * (1.0 - std::pow(1.0 - prob_fail_one_group, g));
  }
  /**
   *
   * @param m          Number of balls
   * @param n          Number of bins
   * @param t          Decoding capacity
   * @param r          Number of rounds
   * @return
   */
  static Mat computeMultiRoundProbabilityMatrix(size_t m, size_t n, size_t t, size_t r) {
    auto m2d = computeTransitionProbabilityMatrix(m, n, t);
    Mat mr_m2d = Mat::Zero(m, r);
    Mat trans_mat_reform = Mat::Zero(m + 1, m + 1);
    trans_mat_reform.block(1, 0, m, m + 1) = m2d.block(1, 1, m, m + 1);
    trans_mat_reform(0, 0) = 1.0;
    Mat temp = trans_mat_reform;
    for (size_t i = 0; i < r; ++i) {
      mr_m2d.col(i) = temp.block(1, 0, m, 1);
      temp = temp * trans_mat_reform;
    }
    Mat mr_m2d_matlab = Mat::Zero(m + 1, r + 1);
    mr_m2d_matlab.block(1, 1, m, r) = Mat::Ones(m, r) - mr_m2d;
    return mr_m2d_matlab;
  }
  /// Compute transition probability matrix
  /**
   *
   * @param m             Number of balls
   * @param n             Number of bins
   * @param t             Decoding capacity
   * @return
   */
  static Mat computeTransitionProbabilityMatrix(size_t m, size_t n, size_t t) {
    Mat m2d = Mat::Zero(m + 1, m + 2);
    auto m3d = computeProbabilityMatrix3D(m, n);

    for (size_t i = 1; i <= m; ++i) {
      for (size_t j = 0; j <= i; ++j) {
        m2d(i, j + 1) = m3d[i].block(n - t, i - j + 1, t + 1, 1).sum();
      }
    }

    for (size_t i = t + 1; i <= m; ++i)
      m2d(i, i + 1) = 1 - m2d.block(i, 1, 1, i).sum();
    return m2d;
  }
  /// Compute probabilities of some specific balls-into-bins events
  /**
   * Computes the probability of each event (i,j,k): throwing i balls into
   * n bins results in that j bins are empty and (k - 1) bins contain exact one
   * ball.
   *
   *  TRANSLATING FROM OUR MATLAB VERSION
   *
   * @param m         Number of balls
   * @param n         Number of bins
   * @return          Probabilities of the event (i,j,k)
   */
  static std::vector<Mat> computeProbabilityMatrix3D(size_t m, size_t n) {
    assert(m < n);
    std::vector<Mat> m3d(m + 1, Mat::Zero(n + 1, m + 2));
    m3d[1](n - 1, 2) = 1.0;
    for (size_t x = 2; x <= m; ++x) {
      for (size_t a = n - m; a < n; ++a) {
        for (size_t b = 1; b <= x + 1; ++b) {
          if (b == 1)
            m3d[x](a, b) =
                m3d[x - 1](a, b + 1) * static_cast<double>(b) / n +
                    m3d[x - 1](a, b) * static_cast<double>(n - a - b + 1) / n;
          else if (b == m + 1)
            m3d[x](a, b) =
                m3d[x - 1](a + 1, b - 1) * static_cast<double>(a + 1) / n +
                    m3d[x - 1](a, b) * static_cast<double>(n - a - b + 1) / n;
          else
            m3d[x](a, b) =
                m3d[x - 1](a + 1, b - 1) * static_cast<double>(a + 1) / n +
                    m3d[x - 1](a, b + 1) * static_cast<double>(b) / n +
                    m3d[x - 1](a, b) * static_cast<double>(n - a - b + 1) / n;
        }
      }
    }
    return m3d;
  }

}; // PbsParam
} // namespace pbsutils

#endif // PBS_PARAMS_H_
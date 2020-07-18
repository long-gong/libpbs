/**
 * @file pbs_params.h
 * @author Long Gong <long.github@gmail.com>
 * @brief The PBS parameter optimization module
 * @note  This file is ONLY a "C++ translation" for the MATLAB codes provided
 * @version 0.1
 * @date 2020-07-15
 *
 * @copyright Copyright (c) 2020
 *
 */
#ifndef PBS_PARAMS_H_
#define PBS_PARAMS_H_

#include <eigen3/Eigen/Dense>
#include <stats.hpp>

#include <vector>

#include "cache_helper.h"

namespace pbsutils {

namespace {
constexpr size_t MAX_BALLS = 200;
constexpr size_t M_MIN = 6;
constexpr size_t M_MAX = 14;
} // end namespace

// matrix type of double
using Mat = Eigen::MatrixXd;

/**
 * @brief BestBchParam struct
 *
 *
 */
struct BestBchParam {
  // n = 2^m - 1 is the block length for this BCH code
  size_t m;
  // error-correcting capacity of this BCH code
  size_t t;
};

/**
 * @brief PbsParam class
 *
 */
class PbsParam {
 public:
  /**
   * @brief Calculate the best BCH parameter (in terms of minimizing communication overhead) in PBS
   *
   * @param d                     cardinality of the set difference (either exact or an accurate estimate)
   * @param delta                 average number of distinct elements per group
   * @param r                     maximum number of rounds to achieve the target success probability
   * @param c                     number of groups to be further partitioned when BCH decoding failed
   * @param targetProb            target success probability for PBS to reconcile all distinct elements in `r` rounds
   * @param bch_param             best parameter to be returned
   * @return                      upper bound for the failure probability when using the best parameter
   */
  static double bestBchParam(size_t d, double delta, size_t r, size_t c,
                             double targetProb, BestBchParam &bch_param) {
    double best_cost = std::numeric_limits<double>::max(), cost = 0;
    size_t m = 1, t = 1;
    double failure_prob_ub = -1.0;

    for (size_t i = M_MIN; i <= M_MAX; ++i) {
      auto t_min = i;
      auto t_max = std::min(
          MAX_BALLS, std::min((1lu << i) - 2lu, size_t(std::ceil(5 * delta))));
      auto j = (1lu << i) - 1;
      auto p_min = 1 - failureProbabilityUB(d, delta, j, r, t_min, c);
      auto p_max = 1 - failureProbabilityUB(d, delta, j, r, t_max, c);

      if (p_min >= targetProb) {
        cost = static_cast<double>(t_min) * i;
        if (cost < best_cost) {
          best_cost = cost;
          m = i;
          t = t_min;
          failure_prob_ub = 1 - p_min;
        }
      } else if (p_max >= targetProb) {
        size_t t_mid = 0;
        while (t_max - t_min > 1u) {
          t_mid = t_min + (t_max - t_min) / 2;
          auto p = 1 - failureProbabilityUB(d, delta, j, r, t_mid, c);
          if (p >= targetProb)
            t_max = t_mid;
          else
            t_min = t_mid;
        }
        auto p = 1 - failureProbabilityUB(d, delta, j, r, t_min, c);
        size_t t_tmp = 0;
        if (p >= targetProb)
          t_tmp = t_min;
        else {
          t_tmp = t_max;
          p = 1 - failureProbabilityUB(d, delta, j, r, t_max, c);
        }
        cost = static_cast<double>(t_tmp) * i;
        if (cost < best_cost) {
          best_cost = cost;
          m = i;
          t = t_tmp;
          failure_prob_ub = 1 - p;
        }
      }
    }

    bch_param.m = m;
    bch_param.t = t;
    return failure_prob_ub;
  }

  /**
   * @brief Compute "times 2 bound" for the failure probability
   *
   * For more details regarding "time2 bound", please refer to Corollary 5.11 on
   * Page 103 in the following book:
   *
   * Mitzenmacher, M. and Upfal, E., 2017. Probability and computing:
   * Randomization and probabilistic techniques in algorithms and data analysis.
   * Cambridge university press.
   *
   * @param mr_m2d        transition probability matrix for multi-round operations in PBS
   * @param m             number of balls (distinct elements)
   * @param n             number of bins
   * @param t             error-correcting capacity
   * @param r             maximum number of rounds to achieve the target success probability
   * @return              "times 2 bound" for the failure probability
   */
  static double computeFailureProbabilityBound(const Mat &mr_m2d, size_t m,
                                               size_t n, size_t t, size_t r) {
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

  /**
   * @brief "times 2 bound" for the failure probability
   *
   * For more details regarding "time2 bound", please refer to Corollary 5.11 on
   * Page 103 in the following book:
   *
   * Mitzenmacher, M. and Upfal, E., 2017. Probability and computing:
   * Randomization and probabilistic techniques in algorithms and data analysis.
   * Cambridge university press.
   *
   * @param d             cardinality of the set difference
   * @param delta         average number of distinct elements per group
   * @param n             block length of BCH code
   * @param r             maximum number of rounds
   * @param t             error-correcting capacity of BCH code
   * @param c             number of groups to further partiton when BCH decoding failed
   * @return              "times 2 bound" for the failure probability
   */
  static double failureProbabilityUB(size_t d, double delta, size_t n, size_t r,
                                     size_t t, size_t c) {
    auto g = (double)d / delta;
    // added @2020-07-17, since stats::dbinom reuqires g >= 1
    if (g < 1) g = 1;
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
      prob_fail_one_group +=
          p * computeFailureProbabilityBound(mr_md, i, c, t, r - 1);
      prob_tail -= p;
    }

    prob_fail_one_group += prob_tail;
    return 2.0 * (1.0 - std::pow(1.0 - prob_fail_one_group, g));
  }
  /**
   * @brief Compute the transition probability for multi-round operations in PBS
   *
   * @param m          number of balls
   * @param n          number of bins
   * @param t          decoding capacity
   * @param r          number of rounds
   * @return           the transition probability
   */
  static Mat computeMultiRoundProbabilityMatrix(size_t m, size_t n, size_t t,
                                                size_t r) {
    {  // loading cache
      if (memcache_check({m, n, t, r})) return memcache_fetch({m, n, t, r});

      Mat cached_mat;
      if (load_cache({m, n, t, r}, cached_mat)) {
        memcache_write({m, n, t, r}, cached_mat);
        return cached_mat;
      }
    }

    Mat mr_m2d = Mat::Zero(m, r);
    auto m2d = computeTransitionProbabilityMatrix(m, n, t);
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

    {
      // caching
      memcache_write({m, n, t, r}, mr_m2d_matlab);
      save_cache({m, n, t, r}, mr_m2d_matlab);
    }

    return mr_m2d_matlab;
  }

  /**
   * @brief Compute transition probability matrix
   *
   * @param m             number of balls
   * @param n             number of bins
   * @param t             error-correcting capacity
   * @return              the transition probability matrix
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

  /**
   * @brief Compute probabilities of some specific balls-into-bins events
   *
   * Computes the probability of each event (i,j,k): throwing i balls into
   * n bins results in that j bins are empty and (k - 1) bins contain exact one
   * ball.
   *
   *
   * @param m         number of balls
   * @param n         number of bins
   * @return          probability matrix (3D) of the event (i,j,k)
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

};  // PbsParam
}  // namespace pbsutils

#endif  // PBS_PARAMS_H_
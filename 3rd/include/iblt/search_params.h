#ifndef SEARCH_PARAMS_H_
#define SEARCH_PARAMS_H_

#include <assert.h>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <vector>

using namespace std;

#define START 0.001
#define NUM 5000
#define CONSIDER_TOBE_ZERO 1e-10

struct csvdata {
  int item;
  double hedge;
  int numhash;
  int size;
  double p;
};

struct search_params {

  vector<struct csvdata> params;

  search_params() {
    FILE *csv;
    char buf[100];
    csv =
//        fopen("../IBLT-optimization/param.export.0.995833.2018-07-17.csv", "r");
    fopen("./param.export.0.995833333333333.2018-07-12.csv", "r");
    fgets(buf, 99, csv);
    struct csvdata tmp;
    while (!feof(csv)) {
      fscanf(csv, "%d,%lf,%d,%d,%lf", &tmp.item, &tmp.hedge, &tmp.numhash,
             &tmp.size, &tmp.p);
      params.push_back(tmp);
    }
    fclose(csv);
  }

  double bf_num_bytes(double error_rate, int capacity) {
    assert(error_rate > 0 && error_rate < 1);
    int num_slices = ceil(-log(error_rate) / log(2));
    int bits_per_slice =
        ceil(capacity * (-log(error_rate)) / (num_slices * log(2) * log(2)));
    return num_slices * bits_per_slice / 8.0;
  }

  double total(double a, double fpr, int n, int y, int &rows) {
    // Total cost of graphene for given parameters
    if (a <= 0)
      printf("a <= 0!\n");
    assert(a > 0);
    // allow not using the BF
    double s = (std::abs(1.0 - fpr) < CONSIDER_TOBE_ZERO) ? 0: bf_num_bytes(fpr, n);
    int i;
    if (ceil(a) + y - 1 >= params.size()) { // difference too much
      double tmp = (ceil(a) + y) * 1.362549;
      rows = ceil(tmp);
      i = rows * 12;
    } else {
      rows = params[ceil(a) + y - 1].size;
      i = rows * 12;
    }
    return s + i; // total
  }

  void solve_a(int m, int n, int x, int y, double &min_a, double &min_fpr,
               int &min_iblt_rows) {
    assert(x <= m);
    double denom;
    if (x == m) {
      denom = 1;
    } else {
      denom = m - x;
    }
    min_a = ceil(START);
    min_fpr = START / denom;
    double min_total = total(START, START / denom, n, y, min_iblt_rows);
    double gap = (denom - START) / NUM;
    double c = START;
    for (int i = 0; i < NUM; ++i) {
      int iblt_rows;
      double t = total(c, c / denom, n, y, iblt_rows);
      if (t < min_total) {
        min_total = t;
        min_a = ceil(c);
        min_fpr = c / denom;
        min_iblt_rows = iblt_rows;
      }
      c += gap;
    }
    if (!(min_fpr <= 1 && min_fpr > 0))
      printf("a out of bounds\n");
    assert(min_fpr <= 1 && min_fpr > 0);
  }

  double CB_bound(double a, double fpr, double bound) {
    double s = -log(bound) / a;
    double temp = sqrt(s * (s + 8));
    double delta_1 = 0.5 * (s + temp);
    double delta_2 = 0.5 * (s - temp);
    assert(delta_1 >= 0);
    assert(delta_2 <= 0);
    (void)(delta_2);
    return (1 + delta_1) * a;
  }

  void CB_solve_a(int m, int n, int x, int y, double bound, double &min_a,
                  double &min_fpr, int &min_iblt_rows) {
    assert(x <= m);
    double denom;
    if (x == m) {
      denom = 1;
    } else {
      denom = m - x;
    }

    min_a = CB_bound(START, START / denom, bound);
    min_fpr = START / denom;
    double min_total = total(min_a, START / denom, n, 0, min_iblt_rows);

    // added by Long
    // since we used very large sets, the original search space
    // seems not large enough.
    int temp_iblt_rows = 0;
    double temp_min_a = CB_bound(denom, 1.0, bound);
    double t_wo_bf = total(temp_min_a, 1.0, n, 0, temp_iblt_rows);
    if (min_total > t_wo_bf) {
      min_fpr = 1.0;
      min_iblt_rows = temp_iblt_rows;
      min_total = t_wo_bf;
      min_a = ceil(temp_min_a);
    }

    double gap = (denom - START) / NUM;
    double c = START;
    for (int i = 0; i < NUM; ++i) {
      int iblt_rows;
      double b = CB_bound(c, c / denom, bound);
      double t = total(b, c / denom, n, 0, iblt_rows);
      if (t < min_total) {
        min_total = t;
        min_a = ceil(b);
        min_fpr = c / denom;
        min_iblt_rows = iblt_rows;
      }
      c += gap;
    }
    if (!(min_fpr <= 1 && min_fpr > 0))
      printf("a out of bounds\n");
    assert(min_fpr <= 1 && min_fpr > 0);
  }

  double compute_delta(int z, int x, int m, double fpr) {
    double temp = (m - x) * fpr;
    temp = (z - x) / temp;
    return temp - 1;
  }

  double compute_RHS(double delta, int m, int x, double fpr) {
    double num = exp(delta);
    double denom = pow(1 + delta, 1 + delta);
    double base = num / denom;
    double exponent = (m - x) * fpr;
    return pow(base, exponent);
  }

  double binom(int n, int k) { // might be slow!!!
    if (k > n)
      return 0;
    if (k * 2 > n)
      k = n - k;
    if (k == 0)
      return 1;

    double result = n;
    for (int i = 2; i <= k; ++i) {
      result *= (n - i + 1);
      result /= i;
    }
    return result;
  }

  double compute_binomial_prob(int m, int x, int z, double fpr) {
    int trials = m - x;
    int success = z - x;
    double term_one = pow(fpr, success);
    double term_two = pow(1 - fpr, trials - success);
    double term_three = (double)binom(trials, success);
    return (term_one * term_two * term_three);
  }

  int search_x_star(int z, int mempool_size, double fpr, double bound,
                    int blk_size) {
    double total = 0;
    int x_star = 0;
    int val = z <= blk_size ? z : blk_size;
    for (int x = 0; x <= val; ++x) {
      double delta = compute_delta(z, x, mempool_size, fpr);
      double result = compute_RHS(delta, mempool_size, x, fpr);
      if (total + result > bound) {
        if (x == 0)
          x_star = x;
        else
          x_star = x - 1;
        break;
      } else
        total += result;
    }
    return x_star;
  }
};

#endif //SEARCH_PARAMS_H_

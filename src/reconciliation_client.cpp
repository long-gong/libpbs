#include <CLI/CLI.hpp>
#include "reconciliation_client.h"


int main(int argc, char **argv) {
  CLI::App app{"Set Reconciliation Benchmark Client"};
  std::string target_str = "localhost:50051";
  app.add_option("--target", target_str, "IP:port");
  std::vector<size_t> diffs{100};
  app.add_option("--diffs", diffs, "Cardinalities of the set difference");
  std::vector<size_t> value_sizes{20};
  app.add_option("--value-sizes", value_sizes, "Sizes (in bytes) of each value");
  size_t uion_sz = 10000;
  app.add_option("--union-size", uion_sz, "Cardinality of the set union");
  unsigned random_seed = 20200721u;
  app.add_option("--seed", random_seed, "Random seed");
  size_t experiments_times = 100;
  app.add_option("--times", experiments_times, "Number of experiments for each combination of parameters");

  fmt::print("{} ... ", "Parsing command line arguments");
  CLI11_PARSE(app, argc, argv);
  fmt::print("{}\n", "done");

  fmt::print("{} ... ", "Create reconciliation client");
  ReconciliationClient reconciliation_client(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  fmt::print("{}\n", "done");

#ifndef TARGET_SUCCESS_PROB
  bool pbs_only = false;
#else
  constexpr double DEFAULT_CB = (1.0 - 239.0 / 240);
  static_assert( std::abs(TARGET_SUCCESS_PROB - DEFAULT_CB) < 1e-5, "TARGET_SUCCESS_PROB should 1/240" );
  bool pbs_only = true;
#endif

  size_t offset = 0;
  for (const auto value_sz: value_sizes) {
    for (const auto d: diffs) {
      fmt::print("Experiments with d = {}, value_sz = {}, union_sz = {} ...\n", d, value_sz, uion_sz);
      reconciliation_client.Reconciliation_Experiments(uion_sz,
                                                       d,
                                                       value_sz,
                                                       random_seed + offset,
                                                       experiments_times,
                                                       pbs_only);
      fmt::print("{}\n", "Done");
    }
  }

  return 0;
}

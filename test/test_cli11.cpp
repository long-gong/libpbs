#include <CLI/CLI.hpp>

//#if __cplusplus >= 201703L
///* Now remove the trow */
//#define throw( \
//    ...)  //  error: ISO C++1z does not allow dynamic exception specifications
//#include <stlcache/stlcache.hpp>
//#undef throw /* reset */
//#else
//#include <stlcache/stlcache.hpp>
//#endif
#include <fmt/format.h>

int main(int argc, char** argv) {
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

  CLI11_PARSE(app, argc, argv);

  fmt::print("{}, {}, {}, {}, {}, {}\n", target_str, fmt::join(diffs.begin(), diffs.end(), " "),
      fmt::join(value_sizes.begin(), value_sizes.end(), " "), uion_sz, random_seed, experiments_times);

  return 0;
}

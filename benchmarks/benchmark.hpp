#pragma once

#include <string>

extern std::string wavefront(
  const std::string& model,
  const unsigned num_threads,
  const unsigned num_rounds
);

extern std::string runBenchmark (
  const std::string& model, 
  const unsigned num_threads, 
  const unsigned num_rounds, 
  const unsigned _inner_loop, 
  const unsigned _workload
);
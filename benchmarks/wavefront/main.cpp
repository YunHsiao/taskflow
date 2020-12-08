#include <CLI11.hpp>
#include "../benchmark.hpp"

int main(int argc, char* argv[]) 
{
  CLI::App app{"Wavefront"};

  unsigned num_threads {12};
  app.add_option("-t,--num_threads", num_threads, "number of threads (default=12)");

  unsigned num_rounds {100};
  app.add_option("-r,--num_rounds", num_rounds, "number of rounds (default=100)");

  unsigned inner_loop {1};
  app.add_option("-i,--inner_loop", inner_loop, "inner loop count (default=1)");
    
  unsigned workload {10};
  app.add_option("-w,--workload", workload, "workload amount (default=10)");

  std::string model = "asio";
  app.add_option("-m,--model", model, "model name seq|omp|tf|asio|tbb|job (default=tf)")
     ->check([] (const std::string& m) {
        if(m != "tbb" && m != "omp" && m != "tf" && m != "asio" && m != "job" && m != "seq") {
          return "model name should be \"tbb\", \"omp\", \"seq\", \"asio\", \"job\", or \"tf\"";
        }
        return "";
     });

  CLI11_PARSE(app, argc, argv);

  std::string info = runBenchmark(model, num_threads, num_rounds, inner_loop, workload);
  std::cout << std::setw(12) << info << std::endl;

  return 0;
}

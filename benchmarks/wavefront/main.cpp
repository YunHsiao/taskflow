#include "matrix.hpp"
#include <CLI11.hpp>

int M = 0;
int N = 0;
int B = 0;
int MB = 0;
int NB = 0;
double **matrix = nullptr;

void wavefront(
  const std::string& model,
  const unsigned num_threads,
  const unsigned num_rounds
  ) {

  std::cout << std::setw(12) << "size"
            << std::setw(12) << "runtime"
            << std::endl;

  for(int S=32; S<=4096; S += 128) {

    M = N = S;
    B = 8;
    MB = (M/B) + (M%B>0);
    NB = (N/B) + (N%B>0);

    double runtime {0.0};

    init_matrix();

    for(unsigned j=0; j<num_rounds; ++j) {
      if(model == "tf") {
        runtime += measure_time_taskflow(num_threads).count();
      }
      else if(model == "tbb") {
        runtime += measure_time_tbb(num_threads).count();
      }
      else if(model == "omp") {
        runtime += measure_time_omp(num_threads).count();
      }
      else if(model == "asio") {
        runtime += measure_time_asio(num_threads).count();
      }
      else if(model == "job") {
        runtime += measure_time_job(num_threads).count();
      }
      else if(model == "seq") {
        runtime += measure_time_seq().count();
      }
      else assert(false);
    }

    destroy_matrix();

    std::cout << std::setw(12) << MB*NB
              << std::setw(12) << runtime / num_rounds / 1e3
              << std::endl;
  }
}

int main(int argc, char* argv[]) {

  CLI::App app{"Wavefront"};

  unsigned num_threads {12};
  app.add_option("-t,--num_threads", num_threads, "number of threads (default=12)");

  unsigned num_rounds {100};
  app.add_option("-r,--num_rounds", num_rounds, "number of rounds (default=100)");

  std::string model = "tf";
  app.add_option("-m,--model", model, "model name tbb|omp|tf|asio|seq|job (default=tf)")
     ->check([] (const std::string& m) {
        if(m != "tbb" && m != "omp" && m != "tf" && m != "asio" && m != "job" && m != "seq") {
          return "model name should be \"tbb\", \"omp\", \"seq\", \"asio\", \"job\", or \"tf\"";
        }
        return "";
     });

  CLI11_PARSE(app, argc, argv);

  std::cout << "model=" << model << ' '
            << "num_threads=" << num_threads << ' '
            << "num_rounds=" << num_rounds << ' '
            << std::endl;

  wavefront(model, num_threads, num_rounds);

  return 0;
}

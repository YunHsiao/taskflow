#include "benchmark.hpp"

#include "wavefront/matrix.hpp"
#include <memory>
#include <string>
#include <stdarg.h>

std::string format(const char* format, ...)
{
#define MAX_STRING_LENGTH (1024*100)
    std::string ret;
    
    va_list ap;
    va_start(ap, format);
    
    char* buf = (char*)malloc(MAX_STRING_LENGTH);
    if (buf != nullptr)
    {
        vsnprintf(buf, MAX_STRING_LENGTH, format, ap);
        ret = buf;
        free(buf);
    }
    va_end(ap);
    return ret;
}

int M = 0;
int N = 0;
int B = 0;
int MB = 0;
int NB = 0;
double **matrix = nullptr;
unsigned inner_loop = 1;
unsigned workload = 10;

std::string wavefront(
  const std::string& model,
  const unsigned num_threads,
  const unsigned num_rounds
  ) {

  std::string result = "size\truntime\n";

  for(int S=32; S<=2048; S += 128) {

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

    result = format("%s\n%d\t%.2f", result.c_str(), MB*NB, runtime / num_rounds / 1e3);
  }
  return result;
}

std::string runBenchmark (
  const std::string& model, 
  const unsigned num_threads, 
  const unsigned num_rounds, 
  const unsigned _inner_loop, 
  const unsigned _workload
  )
{
  inner_loop = _inner_loop;
  workload = _workload;

  std::string result = wavefront(model, num_threads, num_rounds);

  std::string str = format("model=%s num_threads=%d num_rounds=%d\n%s", model.c_str(), num_threads, num_rounds, result.c_str());
  return str;
}
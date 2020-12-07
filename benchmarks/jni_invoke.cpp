#include <jni.h>
#include <string>
#include <CLI11.hpp>
#include "benchmark.hpp"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_parallel_1benchmark_MainActivity_runBenchmark(
        JNIEnv* env,
        jobject /* this */) {

    // seq|omp|tf|asio|tbb|job
    std::string model = "job";
    unsigned num_threads {12};
    unsigned num_rounds {20};
    unsigned inner_loop {1};
    unsigned workload {10};
    
    std::string text = runBenchmark(model, num_threads, num_rounds, inner_loop, workload);

    return env->NewStringUTF(text.c_str());
}
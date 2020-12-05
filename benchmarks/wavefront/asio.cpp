#include "matrix.hpp"

#include <memory>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

std::vector<boost::asio::strand<boost::asio::io_context::executor_type>> strands;
std::allocator<void> allocator;

void compute(int i, int j) {
    block_computation(i, j);
    if (i < MB - 1) strands[i + 1].post(std::bind(compute, i + 1, j), allocator);
}

// wavefront computation
void wavefront_asio(unsigned num_threads) {
    boost::asio::io_context io;
    
    for (int i = 0; i < MB; ++i) {
        strands.emplace_back(boost::asio::make_strand(io.get_executor()));
    }
    
    matrix[M-1][N-1] = 0;

    /* *
    for(int i = 0; i < MB; ++i) {
        for(int j = 0; j < NB; ++j) {
            strands[i].post(std::bind(block_computation, i, j), allocator);
        }
    }
    /* */
    for(int j = 0; j < NB; ++j) {
        strands[0].post(std::bind(compute, 0, j), allocator);
    }
    /* */

    
    /* */
    std::vector<std::thread> threads;
    for (unsigned i = 0; i < num_threads; ++i) {
        threads.emplace_back(std::thread(boost::bind(&boost::asio::io_context::run, &io)));
    }

    for (unsigned i = 0; i < num_threads; ++i) {
        threads[i].join();
    }
    /* *
    io.run();
    /* */

    strands.clear();
}

std::chrono::microseconds measure_time_asio(unsigned num_threads) {
  auto beg = std::chrono::high_resolution_clock::now();
  wavefront_asio(num_threads);
  auto end = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(end - beg);
}

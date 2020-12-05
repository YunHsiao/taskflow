#include "matrix.hpp"

#include "JobSystem.h"

auto timer_job = std::chrono::high_resolution_clock::now();
auto timer_job_snap = std::chrono::high_resolution_clock::now();
// wavefront computation
void wavefront_job(unsigned num_threads, bool first) {
    ThreadPool::kMaxThreadCount = num_threads;
    
    JobSystem sys;
    sys.Start();
    
    JobGraph g;
    
    std::vector<std::vector<JobHandle::Ptr>> handles(MB);
    matrix[M-1][N-1] = 0;
    
    for(int i = 0; i < MB; ++i) {
        handles[i].resize(NB);
        for(int j = 0; j < NB; ++j) {
            auto h = sys.CreateJob("Job", block_computation, i, j);

            handles[i][j] = h;
            if (j-1 >= 0 && i-1 >= 0) g.Schedule(handles[i][j], sys.CombineJobs({ handles[i][j-1], handles[i-1][j] }));
            else if (i-1 >= 0) g.Schedule(handles[i][j], handles[i-1][j]);
            else if (j-1 >= 0) g.Schedule(handles[i][j], handles[i][j-1]);
            else g.Schedule(handles[i][j]);
        }
    }
    
    auto beg = std::chrono::high_resolution_clock::now();
    sys.Dispatch(g);
    handles[MB-1][NB-1]->WaitForComplete();
    auto end = std::chrono::high_resolution_clock::now();
    timer_job += end - beg;
    
    sys.Stop();
}

std::chrono::microseconds measure_time_job(unsigned num_threads) {
  timer_job_snap = timer_job;
  for (unsigned i = 0; i < inner_loop; ++i) {
    wavefront_job(num_threads, i == 0);
  }
  return std::chrono::duration_cast<std::chrono::microseconds>(timer_job - timer_job_snap);
}

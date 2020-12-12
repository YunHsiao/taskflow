cmake_minimum_required(VERSION 3.15)

if("${CMAKE_SYSTEM_NAME}" MATCHES "iOS")
  set(IOS ON)
elseif("${CMAKE_SYSTEM_NAME}" MATCHES "Android")
  set(ANDROID ON)
endif()

# -----------------------------------------------------------------------------
# eigen package
# -----------------------------------------------------------------------------
if(NOT DEFINED EIGEN_ROOT)
  set(EIGEN_ROOT ${PROJECT_SOURCE_DIR}/3rd-party/eigen-3.3.7)
endif()
include_directories(${EIGEN_ROOT})

# -----------------------------------------------------------------------------
# find OpenMP package
# -----------------------------------------------------------------------------

if(ANDROID)
  set(OMPDIR ${PROJECT_SOURCE_DIR}/omp-android)
  set(OpenMP_VERSION 5.0.0)
  set(OpenMP_CXX_FLAGS -I${OMPDIR}/include)
  set(OpenMP_CXX_LIBRARIES ${OMPDIR}/lib/libomp.a)
elseif(IOS)
  set(OMPDIR ${PROJECT_SOURCE_DIR}/omp-ios)
  set(OpenMP_VERSION 5.0.0)
  set(OpenMP_CXX_FLAGS -I${OMPDIR}/include)
  set(OpenMP_CXX_LIBRARIES ${OMPDIR}/lib/libomp.a)
else()
  include(FindOpenMP)

  if(NOT OpenMP_CXX_FOUND)
    message(FATAL_ERROR "OpenMP not found")
  endif()
endif()

message(STATUS "OpenMP_VERSION: ${OpenMP_VERSION}")
message(STATUS "OpenMP_CXX_FLAGS: ${OpenMP_CXX_FLAGS}")
message(STATUS "OpenMP_CXX_LIBRARIES: ${OpenMP_CXX_LIBRARIES}")

# -----------------------------------------------------------------------------
# tbb package
# -----------------------------------------------------------------------------

if(ANDROID)
  set(TBBDIR ${PROJECT_SOURCE_DIR}/tbb-android)
  add_library(tbb STATIC IMPORTED GLOBAL)
  set_target_properties(tbb PROPERTIES
    IMPORTED_LOCATION ${TBBDIR}/lib/libtbb_static.a
    INTERFACE_INCLUDE_DIRECTORIES ${TBBDIR}/include
  )
  add_library(tbbmalloc STATIC IMPORTED GLOBAL)
  set_target_properties(tbbmalloc PROPERTIES
    IMPORTED_LOCATION ${TBBDIR}/lib/libtbbmalloc_static.a
    INTERFACE_INCLUDE_DIRECTORIES ${TBBDIR}/include
  )
  add_library(tbbmalloc_proxy STATIC IMPORTED GLOBAL)
  set_target_properties(tbbmalloc_proxy PROPERTIES
    IMPORTED_LOCATION ${TBBDIR}/lib/libtbbmalloc_proxy_static.a
    INTERFACE_INCLUDE_DIRECTORIES ${TBBDIR}/include
  )
  set(TBB_IMPORTED_TARGETS
    tbbmalloc_proxy tbbmalloc  tbb
  )
elseif(IOS)
  set(TBBDIR ${PROJECT_SOURCE_DIR}/tbb-ios)
  add_library(tbb STATIC IMPORTED GLOBAL)
  set_target_properties(tbb PROPERTIES
    IMPORTED_LOCATION ${TBBDIR}/lib/libtbb_static.a
    INTERFACE_INCLUDE_DIRECTORIES ${TBBDIR}/include
  )
  add_library(tbbmalloc STATIC IMPORTED GLOBAL)
  set_target_properties(tbbmalloc PROPERTIES
    IMPORTED_LOCATION ${TBBDIR}/lib/libtbbmalloc_static.a
    INTERFACE_INCLUDE_DIRECTORIES ${TBBDIR}/include
  )
  add_library(tbbmalloc_proxy STATIC IMPORTED GLOBAL)
  set_target_properties(tbbmalloc_proxy PROPERTIES
    IMPORTED_LOCATION ${TBBDIR}/lib/libtbbmalloc_proxy_static.a
    INTERFACE_INCLUDE_DIRECTORIES ${TBBDIR}/include
  )
  set(TBB_IMPORTED_TARGETS
    tbb tbbmalloc tbbmalloc_proxy
  )
else()
  add_subdirectory(${TF_3RD_PARTY_DIR}/tbb ${CMAKE_BINARY_DIR}/tbb)
endif()

# -----------------------------------------------------------------------------
# Benchmarks
# -----------------------------------------------------------------------------

## benchmark 1: wavefront computing
set(WAVEFRONT_SOURCES
  ${TF_BENCHMARK_DIR}/benchmark.cpp
  ${TF_BENCHMARK_DIR}/wavefront/main.cpp
  ${TF_BENCHMARK_DIR}/wavefront/omp.cpp
  ${TF_BENCHMARK_DIR}/wavefront/tbb.cpp
  ${TF_BENCHMARK_DIR}/wavefront/seq.cpp
  ${TF_BENCHMARK_DIR}/wavefront/asio.cpp
  ${TF_BENCHMARK_DIR}/wavefront/job.cpp
  ${TF_BENCHMARK_DIR}/wavefront/taskflow.cpp
  ${PROJECT_SOURCE_DIR}/3rd-party/JobSystem/Job.cpp
  ${PROJECT_SOURCE_DIR}/3rd-party/JobSystem/JobGraph.cpp
  ${PROJECT_SOURCE_DIR}/3rd-party/JobSystem/JobHandle.cpp
  ${PROJECT_SOURCE_DIR}/3rd-party/JobSystem/JobSystem.cpp
  ${PROJECT_SOURCE_DIR}/3rd-party/JobSystem/WorkStealer.cpp
  ${PROJECT_SOURCE_DIR}/3rd-party/JobSystem/ThreadPool.cpp
  ${PROJECT_SOURCE_DIR}/3rd-party/JobSystem/BatchAllocator.cpp
  ${PROJECT_SOURCE_DIR}/3rd-party/JobSystem/Memory.cpp
  ${PROJECT_SOURCE_DIR}/3rd-party/JobSystem/ConditionVariable.cpp
)

if(ANDROID)
  add_library(wavefront SHARED ${TF_BENCHMARK_DIR}/jni_invoke.cpp ${WAVEFRONT_SOURCES} )
else()
  add_executable(wavefront ${WAVEFRONT_SOURCES})
endif()

target_include_directories(wavefront PRIVATE
  ${PROJECT_SOURCE_DIR}/3rd-party
  ${PROJECT_SOURCE_DIR}/3rd-party/CLI11
  ${PROJECT_SOURCE_DIR}/3rd-party/boost_1_74_0
)
target_link_libraries(
  wavefront
  ${PROJECT_NAME}
  ${TBB_IMPORTED_TARGETS}
  ${OpenMP_CXX_LIBRARIES}
  tf::default_settings
)
set_target_properties(wavefront PROPERTIES COMPILE_FLAGS ${OpenMP_CXX_FLAGS})

## benchmark 2: graph traversal
add_executable(
  graph_traversal
  ${TF_BENCHMARK_DIR}/graph_traversal/main.cpp
  ${TF_BENCHMARK_DIR}/graph_traversal/omp.cpp
  ${TF_BENCHMARK_DIR}/graph_traversal/tbb.cpp
  ${TF_BENCHMARK_DIR}/graph_traversal/seq.cpp
  ${TF_BENCHMARK_DIR}/graph_traversal/taskflow.cpp
)
target_include_directories(graph_traversal PRIVATE ${PROJECT_SOURCE_DIR}/3rd-party/CLI11)
target_link_libraries(
  graph_traversal
  ${PROJECT_NAME}
  ${TBB_IMPORTED_TARGETS}
  ${OpenMP_CXX_LIBRARIES}
  tf::default_settings
)
set_target_properties(graph_traversal PROPERTIES COMPILE_FLAGS ${OpenMP_CXX_FLAGS})

## benchmark 3: binary_tree
add_executable(
  binary_tree
  ${TF_BENCHMARK_DIR}/binary_tree/main.cpp
  ${TF_BENCHMARK_DIR}/binary_tree/tbb.cpp
  ${TF_BENCHMARK_DIR}/binary_tree/omp.cpp
  ${TF_BENCHMARK_DIR}/binary_tree/taskflow.cpp
)
target_include_directories(binary_tree PRIVATE ${PROJECT_SOURCE_DIR}/3rd-party/CLI11)
target_link_libraries(
  binary_tree
  ${PROJECT_NAME}
  ${TBB_IMPORTED_TARGETS}
  ${OpenMP_CXX_LIBRARIES}
  tf::default_settings
)
set_target_properties(binary_tree PROPERTIES COMPILE_FLAGS ${OpenMP_CXX_FLAGS})

## benchmark 4: linear_chain
add_executable(
  linear_chain
  ${TF_BENCHMARK_DIR}/linear_chain/main.cpp
  ${TF_BENCHMARK_DIR}/linear_chain/tbb.cpp
  ${TF_BENCHMARK_DIR}/linear_chain/omp.cpp
  ${TF_BENCHMARK_DIR}/linear_chain/taskflow.cpp
)
target_include_directories(linear_chain PRIVATE ${PROJECT_SOURCE_DIR}/3rd-party/CLI11)
target_link_libraries(
  linear_chain
  ${PROJECT_NAME}
  ${TBB_IMPORTED_TARGETS}
  ${OpenMP_CXX_LIBRARIES}
  tf::default_settings
)
set_target_properties(linear_chain PROPERTIES COMPILE_FLAGS ${OpenMP_CXX_FLAGS})

## benchmark 5: MNIST
add_executable(
  mnist
  ${TF_BENCHMARK_DIR}/mnist/main.cpp
  ${TF_BENCHMARK_DIR}/mnist/omp.cpp
  ${TF_BENCHMARK_DIR}/mnist/tbb.cpp
  ${TF_BENCHMARK_DIR}/mnist/seq.cpp
  ${TF_BENCHMARK_DIR}/mnist/taskflow.cpp
)
target_include_directories(mnist PRIVATE ${PROJECT_SOURCE_DIR}/3rd-party/CLI11)
target_link_libraries(
  mnist
  ${PROJECT_NAME}
  ${TBB_IMPORTED_TARGETS}
  ${OpenMP_CXX_LIBRARIES}
  tf::default_settings
)
set_target_properties(mnist PROPERTIES COMPILE_FLAGS ${OpenMP_CXX_FLAGS})

## benchmark 6: matrix multiplication
add_executable(
  matrix_multiplication
  ${TF_BENCHMARK_DIR}/matrix_multiplication/main.cpp
  ${TF_BENCHMARK_DIR}/matrix_multiplication/omp.cpp
  ${TF_BENCHMARK_DIR}/matrix_multiplication/tbb.cpp
  ${TF_BENCHMARK_DIR}/matrix_multiplication/taskflow.cpp
)
target_include_directories(matrix_multiplication PRIVATE
  ${PROJECT_SOURCE_DIR}/3rd-party/CLI11
)
target_link_libraries(
  matrix_multiplication
  ${PROJECT_NAME}
  ${TBB_IMPORTED_TARGETS}
  ${OpenMP_CXX_LIBRARIES}
  tf::default_settings
)
set_target_properties(matrix_multiplication PROPERTIES COMPILE_FLAGS ${OpenMP_CXX_FLAGS})

## benchmark 7: mandelbrot
add_executable(
  mandelbrot
  ${TF_BENCHMARK_DIR}/mandelbrot/main.cpp
  ${TF_BENCHMARK_DIR}/mandelbrot/omp.cpp
  ${TF_BENCHMARK_DIR}/mandelbrot/tbb.cpp
  ${TF_BENCHMARK_DIR}/mandelbrot/taskflow.cpp
)
target_include_directories(mandelbrot PRIVATE ${PROJECT_SOURCE_DIR}/3rd-party/CLI11)
target_link_libraries(
  mandelbrot
  ${PROJECT_NAME}
  ${TBB_IMPORTED_TARGETS}
  ${OpenMP_CXX_LIBRARIES}
  tf::default_settings
)
set_target_properties(mandelbrot PROPERTIES COMPILE_FLAGS ${OpenMP_CXX_FLAGS})

## benchmark 8: black_scholes
add_executable(
  black_scholes
  ${TF_BENCHMARK_DIR}/black_scholes/main.cpp
  ${TF_BENCHMARK_DIR}/black_scholes/omp.cpp
  ${TF_BENCHMARK_DIR}/black_scholes/tbb.cpp
  ${TF_BENCHMARK_DIR}/black_scholes/taskflow.cpp
)
target_include_directories(black_scholes PRIVATE ${PROJECT_SOURCE_DIR}/3rd-party/CLI11)
target_link_libraries(
  black_scholes
  ${PROJECT_NAME}
  ${TBB_IMPORTED_TARGETS}
  ${OpenMP_CXX_LIBRARIES}
  tf::default_settings
)
set_target_properties(black_scholes PROPERTIES COMPILE_FLAGS ${OpenMP_CXX_FLAGS})

## benchmark 9: reduce_sum
add_executable(
  reduce_sum
  ${TF_BENCHMARK_DIR}/reduce_sum/main.cpp
  ${TF_BENCHMARK_DIR}/reduce_sum/omp.cpp
  ${TF_BENCHMARK_DIR}/reduce_sum/tbb.cpp
  ${TF_BENCHMARK_DIR}/reduce_sum/taskflow.cpp
)
target_include_directories(reduce_sum PRIVATE ${PROJECT_SOURCE_DIR}/3rd-party/CLI11)
target_link_libraries(
  reduce_sum
  ${PROJECT_NAME}
  ${TBB_IMPORTED_TARGETS}
  ${OpenMP_CXX_LIBRARIES}
  tf::default_settings
)
set_target_properties(reduce_sum PROPERTIES COMPILE_FLAGS ${OpenMP_CXX_FLAGS})

## benchmark 10: parallel_sort
add_executable(
  parallel_sort
  ${TF_BENCHMARK_DIR}/parallel_sort/main.cpp
  ${TF_BENCHMARK_DIR}/parallel_sort/omp.cpp
  ${TF_BENCHMARK_DIR}/parallel_sort/tbb.cpp
  ${TF_BENCHMARK_DIR}/parallel_sort/taskflow.cpp
)
target_include_directories(parallel_sort PRIVATE ${PROJECT_SOURCE_DIR}/3rd-party/CLI11)
target_link_libraries(
  parallel_sort
  ${PROJECT_NAME}
  ${TBB_IMPORTED_TARGETS}
  ${OpenMP_CXX_LIBRARIES}
  tf::default_settings
)
set_target_properties(parallel_sort PROPERTIES COMPILE_FLAGS ${OpenMP_CXX_FLAGS})

###############################################################################
# CUDA benchmarks
###############################################################################

if(TF_BUILD_CUDA)

  ## cuda benchmark 1: heterogeneous traversal
  add_executable(
    hetero_traversal
    ${TF_BENCHMARK_DIR}/hetero_traversal/main.cu
    ${TF_BENCHMARK_DIR}/hetero_traversal/taskflow.cu
    ${TF_BENCHMARK_DIR}/hetero_traversal/tbb.cu
    ${TF_BENCHMARK_DIR}/hetero_traversal/omp.cu
  )

  target_include_directories(
    hetero_traversal PRIVATE ${PROJECT_SOURCE_DIR}/3rd-party/CLI11
  )

  target_link_libraries(
    hetero_traversal
    ${PROJECT_NAME}
    Threads::Threads
    ${TBB_IMPORTED_TARGETS}
    ${OpenMP_CXX_LIBRARIES}
    tf::default_settings
  )

  set_target_properties(
    hetero_traversal PROPERTIES COMPILE_FLAGS "-Xcompiler ${OpenMP_CXX_FLAGS}"
  )

  # avoid cmake 3.18+ warning
  # we let nvcc to decide the flag if the architecture is not given
  if(NOT CUDA_ARCHITECTURES)
    set_property(TARGET hetero_traversal PROPERTY CUDA_ARCHITECTURES OFF)
  endif()


endif(TF_BUILD_CUDA)

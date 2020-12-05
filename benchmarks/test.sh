cp ../build/benchmarks/Release/wavefront wavefront
./benchmarks.py -m tf tbb omp asio job seq -b wavefront -t 12 -r 1 -i 3 -w 0 -p true -o result.png

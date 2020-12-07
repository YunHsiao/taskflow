#!/bin/bash -x
cmake .. -GXcode \
 -DCMAKE_SYSTEM_NAME=iOS \
 -DCMAKE_OSX_SYSROOT=iphoneos \
 -DTF_BUILD_BENCHMARKS=ON \
 -DOMPDIR=/Users/panda/StudyWork/Cocos/taskflow/omp-ios \
 -DTBBDIR=/Users/panda/StudyWork/Cocos/taskflow/tbb-ios

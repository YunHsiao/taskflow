#!/bin/bash
cmake .. -GNinja \
 -DOPENMP_STANDALONE_BUILD=ON \
 -DCMAKE_SYSTEM_NAME=iOS \
 -DCMAKE_OSX_SYSROOT=iphoneos \
 -DCMAKE_OSX_ARCHITECTURES=arm64 \
 -DCMAKE_C_COMPILER=clang \
 -DCMAKE_CXX_COMPILER=clang++ \
 -DCMAKE_BUILD_TYPE=Release \
 -DLIBOMP_ENABLE_SHARED=OFF \
 -DCMAKE_INSTALL_PREFIX=/Users/panda/StudyWork/Cocos/taskflow/3rd-party/openmp/dst/
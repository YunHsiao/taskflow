#!/bin/bash
cmake .. -GNinja \
 -DCMAKE_MAKE_PROGRAM=/Users/panda/StudyWork/Cocos/tools/adt-bundle/sdk/cmake/3.10.2.4988404/bin/ninja \
 -DCMAKE_SYSTEM_NAME=iOS \
 -DCMAKE_OSX_SYSROOT=iphoneos \
 -DCMAKE_OSX_ARCHITECTURES=arm64 \
 -DCMAKE_C_COMPILER=clang \
 -DCMAKE_CXX_COMPILER=clang++ \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_INSTALL_PREFIX=/Users/panda/StudyWork/Cocos/taskflow/3rd-party/tbb/dst/ios

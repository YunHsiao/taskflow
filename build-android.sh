#!/bin/bash -x
#export ANDROID_NDK_L=$ANDROID_HOME/ndk-bundle
export ANDROID_NDK_L=$ANDROID_NDK
#CMAKE_EXE=$ANDROID_HOME/cmake/3.10.2.4988404/bin/cmake
CMAKE_EXE=cmake
PATH=$PATH:$ANDROID_NDK_L ${CMAKE_EXE} .. "-DCMAKE_CXX_FLAGS=-frtti -fexceptions -fsigned-char" \
    "-DCMAKE_BUILD_TYPE=Release" \
    "-GNinja" \
    "-DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_L/build/cmake/android.toolchain.cmake" \
    "-DANDROID_ABI=arm64-v8a" \
    "-DANDROID_NDK=$ANDROID_NDK_L" \
    "-DANDROID_PLATFORM=26" \
    "-DCMAKE_ANDROID_NDK=$ANDROID_NDK_L" \
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON" \
    "-DCMAKE_MAKE_PROGRAM=/usr/local/bin/ninja" \
    "-DCMAKE_SYSTEM_NAME=Android" \
    "-DCMAKE_SYSTEM_VERSION=26" \
    "-DANDROID_STL=c++_static" \
    "-DANDROID_TOOLCHAIN=clang" \
    "-DCMAKE_BUILD_TYPE=Release" \
    "-DCMAKE_CXX_STANDARD=17"

#!/bin/bash -x
cmake .. -GXcode \
 -DCMAKE_SYSTEM_NAME=iOS \
 -DCMAKE_OSX_SYSROOT=iphoneos

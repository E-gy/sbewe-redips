#!/bin/bash
BUILD_TYPE=Debug
cmake -S . -B build -DCMAKE_BUILD_TYPE=$BUILD_TYPE
cmake --build build --config $BUILD_TYPE

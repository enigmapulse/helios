#!/bin/bash

echo "Building in RELEASE Mode (-O3 Optimizations)..."

mkdir -p build
cd build

# Tell CMake to strictly use Release mode
cmake -DCMAKE_BUILD_TYPE=Release ..

cmake --build . -j$(nproc)

echo "Release Build Complete."
#!/bin/bash
set -e 

echo "Building HFT Engine..."

mkdir -p build
cd build

cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

make -j$(nproc)

echo "Build complete!"
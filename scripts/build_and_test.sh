#!/bin/bash
set -e

./scripts/build.sh

echo "Running Tests..."
cd build

./run_tests
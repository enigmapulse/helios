#!/bin/bash
set -e

./scripts/build.sh

echo "========================================"
echo "Starting HFT Engine..."
echo "========================================"

./build/hft_engine
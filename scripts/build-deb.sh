#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "=== Building WebApp Converter ==="
sudo apt-get install -y build-essential cmake qt6-base-dev 2>/dev/null || true

rm -rf "$BUILD_DIR" && mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build . --parallel "$(nproc)"
cpack -G DEB
echo "Package: $(ls *.deb)"

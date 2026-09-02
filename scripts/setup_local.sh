#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"

if ! command -v brew >/dev/null 2>&1; then
  echo "Homebrew is required on macOS. Install it from https://brew.sh/" >&2
  exit 1
fi

brew install drogon jwt-cpp libpqxx openssl@3

if [[ ! -d Bcrypt.cpp ]]; then
  git clone --depth 1 https://github.com/hilch/Bcrypt.cpp.git Bcrypt.cpp
fi

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)"
cmake --build build --parallel
ctest --test-dir build --output-on-failure

echo "Local build and unit tests completed successfully."

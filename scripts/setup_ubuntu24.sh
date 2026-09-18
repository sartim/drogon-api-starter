#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"

DROGON_TAG=v1.9.13
DROGON_COMMIT=4c5430757ea5451a7c38fbbef4b4bef7dbb47f2f
JWT_CPP_TAG=v0.7.2
JWT_CPP_COMMIT=b0ea29a58fc852a67d4e896d266880c2c63b0c4c
BCRYPT_CPP_COMMIT=0d18b6a99e8c57627910db4ef9a7706c009b12ad

if [[ "$(uname -s)" != "Linux" ]] || ! grep -q 'VERSION_ID="24.04"' /etc/os-release; then
  echo "This script requires Ubuntu 24.04." >&2
  exit 1
fi

sudo apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y \
  build-essential cmake curl git pkg-config \
  libjsoncpp-dev uuid-dev libssl-dev zlib1g-dev libbz2-dev liblzma-dev \
  libpq-dev libpqxx-dev libbrotli-dev libhiredis-dev postgresql postgresql-contrib

if command -v systemctl >/dev/null 2>&1; then
  sudo systemctl start postgresql
else
  sudo service postgresql start
fi

sudo -u postgres createuser --createdb "$(id -un)" 2>/dev/null || true
sudo -u postgres createdb -O "$(id -un)" drogon_user_service 2>/dev/null || true

if [[ ! -d Bcrypt.cpp ]]; then
  git clone --depth 1 https://github.com/hilch/Bcrypt.cpp.git Bcrypt.cpp
fi
test "$(git -C Bcrypt.cpp rev-parse HEAD)" = "$BCRYPT_CPP_COMMIT" || {
  echo "Bcrypt.cpp is not pinned to $BCRYPT_CPP_COMMIT; remove it and rerun setup." >&2
  exit 1
}

if [[ ! -d jwt-cpp ]]; then
  git clone --depth 1 --branch "$JWT_CPP_TAG" https://github.com/Thalhammer/jwt-cpp.git jwt-cpp
fi
test "$(git -C jwt-cpp rev-parse HEAD)" = "$JWT_CPP_COMMIT" || {
  echo "jwt-cpp is not pinned to $JWT_CPP_TAG; remove it and rerun setup." >&2
  exit 1
}

if [[ ! -d drogon ]]; then
  git clone --depth 1 --branch "$DROGON_TAG" --recurse-submodules \
    https://github.com/drogonframework/drogon.git drogon
fi
test "$(git -C drogon rev-parse HEAD)" = "$DROGON_COMMIT" || {
  echo "Drogon is not pinned to $DROGON_TAG; remove it and rerun setup." >&2
  exit 1
}

local_prefix="$project_dir/.local"

cmake -S drogon -B drogon/build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$local_prefix" \
  -DBUILD_POSTGRESQL=ON -DBUILD_MYSQL=OFF -DBUILD_SQLITE=OFF \
  -DBUILD_REDIS=ON \
  -DBUILD_EXAMPLES=OFF -DBUILD_CTL=ON
cmake --build drogon/build --parallel
cmake --install drogon/build

cmake -S jwt-cpp -B jwt-cpp/build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$local_prefix"
cmake --build jwt-cpp/build --parallel
cmake --install jwt-cpp/build

if [[ ! -f .env ]]; then
  cat > .env <<EOF
SECRET_KEY=$(openssl rand -hex 32)
DB_HOST=127.0.0.1
DB_PORT=5432
DB_NAME=drogon_user_service
DB_USER=$(id -un)
DB_PASSWORD=
REDIS_ENABLED=false
REDIS_HOST=127.0.0.1
REDIS_PORT=6379
REDIS_PASSWORD=
REDIS_DB=0
EOF
  echo "Created local .env. Review database settings before starting the server."
fi

cmake --fresh -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_PREFIX_PATH="$local_prefix"
cmake --build build --parallel
ctest --test-dir build --output-on-failure

echo "Ubuntu 24.04 build and unit tests completed successfully."
echo "Server is not running. Start it with: ./build/drogon_user_service --action=run-server"

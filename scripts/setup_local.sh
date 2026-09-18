#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"

DROGON_TAG=v1.9.13
DROGON_COMMIT=4c5430757ea5451a7c38fbbef4b4bef7dbb47f2f
JWT_CPP_TAG=v0.7.2
JWT_CPP_COMMIT=b0ea29a58fc852a67d4e896d266880c2c63b0c4c
BCRYPT_CPP_COMMIT=0d18b6a99e8c57627910db4ef9a7706c009b12ad

if ! command -v brew >/dev/null 2>&1; then
  echo "Homebrew is required on macOS. Install it from https://brew.sh/" >&2
  exit 1
fi

brew install hiredis jsoncpp libpqxx openssl@3 postgresql@16

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

jwt_install_dir="$project_dir/.local"
export PKG_CONFIG_PATH="$(brew --prefix libpqxx)/lib/pkgconfig:$(brew --prefix libpq)/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"

if [[ ! -d drogon ]]; then
  git clone --depth 1 --branch "$DROGON_TAG" --recurse-submodules \
    https://github.com/drogonframework/drogon.git drogon
fi
test "$(git -C drogon rev-parse HEAD)" = "$DROGON_COMMIT" || {
  echo "Drogon is not pinned to $DROGON_TAG; remove it and rerun setup." >&2
  exit 1
}

cmake -S drogon -B drogon/build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$jwt_install_dir" \
  -DBUILD_POSTGRESQL=ON -DBUILD_MYSQL=OFF -DBUILD_SQLITE=OFF \
  -DBUILD_REDIS=ON \
  -DBUILD_EXAMPLES=OFF -DBUILD_CTL=ON
cmake --build drogon/build --parallel
cmake --install drogon/build

cmake -S jwt-cpp -B jwt-cpp/build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$jwt_install_dir"
cmake --build jwt-cpp/build --parallel
cmake --install jwt-cpp/build

if [[ ! -f .env ]]; then
  secret_key="$("$(brew --prefix openssl@3)/bin/openssl" rand -hex 32)"
  cat > .env <<EOF
SECRET_KEY=${secret_key}
DB_HOST=127.0.0.1
DB_PORT=5432
DB_NAME=drogon_user_service
DB_USER=$(whoami)
  DB_PASSWORD=
REDIS_ENABLED=false
REDIS_HOST=127.0.0.1
REDIS_PORT=6379
REDIS_PASSWORD=
REDIS_DB=0
EOF
  echo "Created local .env with a generated development secret."
fi

brew services start postgresql@16 >/dev/null || true
"$(brew --prefix postgresql@16)/bin/createdb" drogon_user_service 2>/dev/null || true

cmake --fresh -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)" \
  -DCMAKE_PREFIX_PATH="$(brew --prefix libpqxx);$jwt_install_dir"
cmake --build build --parallel
ctest --test-dir build --output-on-failure

echo "Local build and unit tests completed successfully."
echo "Server is not running. Start it with: ./build/drogon_user_service --action=run-server"

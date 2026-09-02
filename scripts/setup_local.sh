#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"

if ! command -v brew >/dev/null 2>&1; then
  echo "Homebrew is required on macOS. Install it from https://brew.sh/" >&2
  exit 1
fi

brew install jsoncpp libpqxx openssl@3 postgresql@16

if [[ ! -d Bcrypt.cpp ]]; then
  git clone --depth 1 https://github.com/hilch/Bcrypt.cpp.git Bcrypt.cpp
fi

if [[ ! -d jwt-cpp ]]; then
  git clone --depth 1 https://github.com/Thalhammer/jwt-cpp.git jwt-cpp
fi

jwt_install_dir="$project_dir/.local"
export PKG_CONFIG_PATH="$(brew --prefix libpqxx)/lib/pkgconfig:$(brew --prefix libpq)/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"

if [[ ! -d drogon ]]; then
  git clone --depth 1 --recurse-submodules https://github.com/drogonframework/drogon.git drogon
fi

cmake -S drogon -B drogon/build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$jwt_install_dir" \
  -DBUILD_POSTGRESQL=ON -DBUILD_MYSQL=OFF -DBUILD_SQLITE=OFF \
  -DBUILD_EXAMPLES=OFF -DBUILD_CTL=OFF
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
EOF
  echo "Created local .env with a generated development secret."
fi

brew services start postgresql@16 >/dev/null || true
"$(brew --prefix postgresql@16)/bin/createdb" drogon_user_service 2>/dev/null || true

cmake --fresh -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)" \
  -DCMAKE_PREFIX_PATH="$(brew --prefix libpqxx);$jwt_install_dir"
cmake --build build --parallel
ctest --test-dir build --output-on-failure

echo "Local build and unit tests completed successfully."
echo "Server is not running. Start it with: ./build/drogon_user_service --action=run-server"

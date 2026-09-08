#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
temporary_root="$(mktemp -d)"
trap 'rm -rf "$temporary_root"' EXIT

assert_file() {
  [[ -f "$1" ]] || {
    echo "Missing generated file: $1" >&2
    exit 1
  }
}

assert_no_placeholders() {
  local found=false
  if command -v rg >/dev/null 2>&1; then
    rg --hidden --glob '!.git/**' '@PROJECT_NAME@|drogon_user_service' "$1" && found=true || true
  else
    grep -R -n -E --exclude-dir=.git '@PROJECT_NAME@|drogon_user_service' "$1" && found=true || true
  fi
  if [[ "$found" == true ]]; then
    echo "Generated project contains an unresolved placeholder: $1" >&2
    exit 1
  fi
}

assert_contains() {
  if command -v rg >/dev/null 2>&1; then
    rg -q "$1" "$2"
  else
    grep -q -E "$1" "$2"
  fi
}

build_and_test_generated_project() {
  local project_directory=$1
  local preset=$2

  local cmake_args=(-S "$project_directory")
  if [[ -d "$project_root/.local" ]]; then
    # Native setup_local.sh installs package config files in this optional
    # prefix. Hosted CI uses the system prefix installed by the build image.
    cmake_args+=("-DCMAKE_PREFIX_PATH=$project_root/.local")
  fi
  cmake --preset "$preset" "${cmake_args[@]}"
  local build_directory="$project_directory/build"
  if [[ "$preset" == "user-service" ]]; then
    build_directory+="/user-service"
  fi
  cmake --build "$build_directory" --parallel
  ctest --test-dir "$build_directory" --output-on-failure
}

minimal="$temporary_root/minimal"
"$project_root/scripts/drogon-starter" init payments-api "$minimal" --profile minimal
for file in CMakeLists.txt CMakePresets.json README.md app/main.cc tests/smoke_test.cc; do
  assert_file "$minimal/$file"
  expected="$temporary_root/expected-${file//\//-}"
  sed 's/@PROJECT_NAME@/payments-api/g' "$project_root/templates/minimal/$file" > "$expected"
  diff --unified=3 "$expected" "$minimal/$file"
done
for directory in platform examples migrations deploy; do
  assert_file "$minimal/$directory/.gitkeep"
done
assert_no_placeholders "$minimal"

# A generated starter must be more than a correctly-shaped directory. Build it
# from its own source root so CI catches missing files, broken package lookup,
# and profile/preset drift before a release is published.
build_and_test_generated_project "$minimal" dev

explicit_path="$temporary_root/explicit-path"
"$project_root/scripts/drogon-starter" init reports-api \
  --path "$explicit_path" --profile minimal
assert_file "$explicit_path/CMakeLists.txt"
assert_no_placeholders "$explicit_path"

upgraded="$temporary_root/upgraded"
"$project_root/scripts/drogon-starter" init payments-api "$upgraded" --profile minimal
"$project_root/scripts/drogon-starter" enable user-service "$upgraded" --force
assert_file "$upgraded/controllers/UserController.cc"
assert_file "$upgraded/services/UserService.cc"
assert_no_placeholders "$upgraded"
assert_contains '"profile":"user-service"' "$upgraded/.drogon-starter.json"

users="$temporary_root/users"
"$project_root/scripts/drogon-starter" init users-api "$users"
for file in CMakeLists.txt CMakePresets.json README.md main.cc test/CMakeLists.txt; do
  assert_file "$users/$file"
  expected="$temporary_root/user-expected-${file//\//-}"
  git -C "$project_root" show "HEAD:$file" \
    | sed 's/@PROJECT_NAME@/users-api/g; s/drogon_model::drogon_user_service/drogon_model::users_api/g; s/namespace drogon_user_service/namespace users_api/g; s/drogon_user_service/users-api/g' \
    > "$expected"
  diff --unified=3 "$expected" "$users/$file"
done
for directory in platform examples migrations deploy; do
  assert_file "$users/$directory/.gitkeep"
done
assert_no_placeholders "$users"

# The user-service profile is generated from the maintained implementation.
# Bcrypt.cpp is intentionally a build dependency rather than a committed
# subtree, so make it available in the isolated generated project when the
# checkout provides it (as the Docker CI image does).
if [[ -d "$project_root/Bcrypt.cpp" ]]; then
  cp -R "$project_root/Bcrypt.cpp" "$users/"
fi
[[ -d "$users/Bcrypt.cpp" ]] || {
  echo "Missing Bcrypt.cpp dependency for generated user-service build." >&2
  exit 1
}
build_and_test_generated_project "$users" user-service

if command -v drogon_ctl >/dev/null 2>&1; then
  components="$temporary_root/components"
  mkdir -p "$components"
  "$project_root/scripts/drogon-generate" controller TestController \
    --output "$components/controllers"
  "$project_root/scripts/drogon-generate" filter TestFilter \
    --output "$components/filters"
  assert_file "$components/controllers/TestController.h"
  assert_file "$components/controllers/TestController.cc"
  assert_file "$components/filters/TestFilter.h"
  assert_file "$components/filters/TestFilter.cc"
  echo "drogon_ctl component generation passed."
fi

echo "Generated minimal and user-service profiles match the starter layout."

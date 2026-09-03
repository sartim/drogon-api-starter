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
  if rg --hidden --glob '!.git/**' '@PROJECT_NAME@|drogon_user_service' "$1"; then
    echo "Generated project contains an unresolved placeholder: $1" >&2
    exit 1
  fi
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

users="$temporary_root/users"
"$project_root/scripts/drogon-starter" init users-api "$users"
for file in CMakeLists.txt CMakePresets.json README.md main.cc test/CMakeLists.txt; do
  assert_file "$users/$file"
  expected="$temporary_root/user-expected-${file//\//-}"
  git -C "$project_root" show "HEAD:$file" \
    | sed 's/@PROJECT_NAME@/users-api/g; s/drogon_user_service/users-api/g' \
    > "$expected"
  diff --unified=3 "$expected" "$users/$file"
done
for directory in platform examples migrations deploy; do
  assert_file "$users/$directory/.gitkeep"
done
assert_no_placeholders "$users"

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

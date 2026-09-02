#!/usr/bin/env bash
set -euo pipefail

base_url="${BASE_URL:-http://127.0.0.1:8000}"

assert_status() {
  local expected="$1"
  local path="$2"
  local actual
  actual="$(curl --silent --show-error --output /tmp/drogon-api-response \
    --write-out '%{http_code}' "${base_url}${path}")"
  if [[ "$actual" != "$expected" ]]; then
    echo "Expected ${path} to return ${expected}, got ${actual}" >&2
    cat /tmp/drogon-api-response >&2
    exit 1
  fi
}

assert_status 200 /health
assert_status 200 /ready
assert_status 200 /metrics
assert_status 200 /openapi.yaml
assert_status 200 /swagger.html
assert_status 401 /api/v1/user
assert_status 401 /api/v1/role

echo "Integration smoke checks passed for ${base_url}"

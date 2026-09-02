#!/usr/bin/env bash
set -euo pipefail

base_url="${BASE_URL:-http://127.0.0.1:8000}"
expected_ready="${EXPECTED_READY_STATUS:-200}"
readiness_only="${READINESS_ONLY:-false}"
response_file="$(mktemp)"
trap 'rm -f "$response_file"' EXIT

assert_status() {
  local expected="$1"
  shift
  local actual
  actual="$(curl --silent --show-error --output "$response_file" \
    --write-out '%{http_code}' "$@")"
  if [[ "$actual" != "$expected" ]]; then
    echo "Expected request to return ${expected}, got ${actual}" >&2
    cat "$response_file" >&2
    exit 1
  fi
}

assert_status "$expected_ready" "${base_url}/ready"

if [[ "$readiness_only" == "true" ]]; then
  echo "Readiness degradation check passed for ${base_url}"
  exit 0
fi

auth_email="${INTEGRATION_AUTH_EMAIL:-integration@example.test}"
auth_password="${INTEGRATION_AUTH_PASSWORD:-integration-password}"
assert_status 200 "${base_url}/api/v1/generate-jwt" \
  --header 'Content-Type: application/json' \
  --data "{\"email\":\"${auth_email}\",\"password\":\"${auth_password}\"}"
token="$(jq --raw-output --exit-status '.access' "$response_file")"

assert_status 200 "${base_url}/api/v1/user?page=1&page_size=10" \
  --header "Authorization: Bearer ${token}"
jq --exit-status '.results | length >= 1' "$response_file" >/dev/null

echo "Redis fallback check passed for ${base_url}"

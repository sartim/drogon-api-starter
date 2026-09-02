#!/usr/bin/env bash
set -euo pipefail

base_url="${BASE_URL:-http://127.0.0.1:8000}"
auth_email="${INTEGRATION_AUTH_EMAIL:-integration@example.test}"
auth_password="${INTEGRATION_AUTH_PASSWORD:-integration-password}"
response_file="$(mktemp)"
trap 'rm -f "$response_file"' EXIT

request_status() {
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

request_status 200 "${base_url}/health"
request_status 200 "${base_url}/ready"
request_status 200 "${base_url}/metrics"
request_status 200 "${base_url}/openapi.yaml"
request_status 200 "${base_url}/swagger.html"
request_status 401 "${base_url}/api/v1/user"
request_status 401 "${base_url}/api/v1/role"

request_status 200 "${base_url}/api/v1/generate-jwt" \
  --header 'Content-Type: application/json' \
  --data "{\"email\":\"${auth_email}\",\"password\":\"${auth_password}\"}"
token="$(jq --raw-output --exit-status '.access' "$response_file")"

created_email="integration-${GITHUB_RUN_ID:-local}@example.test"
request_status 201 "${base_url}/api/v1/user" \
  --header "Authorization: Bearer ${token}" \
  --header 'Content-Type: application/json' \
  --data "{\"first_name\":\"Integration\",\"last_name\":\"Test\",\"email\":\"${created_email}\",\"password\":\"created-password\"}"
created_id="$(jq --raw-output --exit-status '.id' "$response_file")"

request_status 200 "${base_url}/api/v1/user?page=1&page_size=10" \
  --header "Authorization: Bearer ${token}"
jq --exit-status '.results | length >= 1' "$response_file" >/dev/null

request_status 200 "${base_url}/api/v1/user/${created_id}" \
  --header "Authorization: Bearer ${token}"
jq --exit-status --arg email "$created_email" '.email == $email' "$response_file" >/dev/null

request_status 200 "${base_url}/api/v1/user/${created_id}" \
  --request PUT \
  --header "Authorization: Bearer ${token}" \
  --header 'Content-Type: application/json' \
  --data "{\"first_name\":\"Updated\",\"last_name\":\"Integration\",\"email\":\"${created_email}\",\"password\":\"updated-password\"}"

request_status 204 "${base_url}/api/v1/user/${created_id}" \
  --request DELETE \
  --header "Authorization: Bearer ${token}"

echo "Authenticated integration checks passed for ${base_url}"

#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"

if [[ ! -f .env ]]; then
  cp .env.example .env
  echo "Created .env from .env.example; review credentials before production use."
fi

build_args=()
if [[ "${COMPOSE_BUILD:-true}" == "true" ]]; then
  build_args+=(--build)
fi

docker compose up "${build_args[@]}" --detach

for attempt in {1..30}; do
  if curl --fail --silent http://127.0.0.1:8000/health >/dev/null; then
    curl --fail --silent http://127.0.0.1:8000/ready >/dev/null
    echo "Drogon API is ready at http://127.0.0.1:8000"
    exit 0
  fi
  sleep 2
done

docker compose logs web-server
exit 1

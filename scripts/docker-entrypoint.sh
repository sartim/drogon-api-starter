#!/bin/sh
set -eu

umask 077
cat > /app/.env <<EOF
SECRET_KEY=${SECRET_KEY:-}
DB_HOST=${DB_HOST:-127.0.0.1}
DB_PORT=${DB_PORT:-5432}
DB_NAME=${DB_NAME:-drogon_user_service}
DB_USER=${DB_USER:-}
DB_PASSWORD=${DB_PASSWORD:-}
SENTRY_DSN=${SENTRY_DSN:-}
HTTP_HOST=${HTTP_HOST:-0.0.0.0}
HTTP_PORT=${HTTP_PORT:-8000}
EOF

exec "$@"

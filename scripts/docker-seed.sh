#!/bin/sh
set -eu

: "${DB_HOST:?DB_HOST is required}"
: "${DB_NAME:?DB_NAME is required}"
: "${DB_USER:?DB_USER is required}"
export PGPASSWORD="${DB_PASSWORD:-}"

psql -h "$DB_HOST" -p "${DB_PORT:-5432}" -U "$DB_USER" -d "$DB_NAME" \
  --set ON_ERROR_STOP=1 --single-transaction \
  --file /db/seeds/001_reference_data.sql

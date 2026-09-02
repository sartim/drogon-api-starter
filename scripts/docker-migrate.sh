#!/bin/sh
set -eu

: "${DB_HOST:?DB_HOST is required}"
: "${DB_NAME:?DB_NAME is required}"
: "${DB_USER:?DB_USER is required}"
export PGPASSWORD="${DB_PASSWORD:-}"

psql_args="-h $DB_HOST -p ${DB_PORT:-5432} -U $DB_USER -d $DB_NAME"
psql $psql_args --set ON_ERROR_STOP=1 <<'SQL'
CREATE TABLE IF NOT EXISTS public.schema_migrations (
    version TEXT PRIMARY KEY,
    applied_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);
SQL

for migration in /db/migrations/*.sql; do
  [ -f "$migration" ] || continue
  version="$(basename "$migration" .sql)"
  applied="$(psql $psql_args --tuples-only --no-align \
    --set ON_ERROR_STOP=1 \
    --command "SELECT 1 FROM public.schema_migrations WHERE version = '$version'")"
  if [ "$applied" = "1" ]; then
    echo "Skipping $version"
    continue
  fi
  echo "Applying $version"
  psql $psql_args --set ON_ERROR_STOP=1 --single-transaction \
    --file "$migration" \
    --command "INSERT INTO public.schema_migrations(version) VALUES ('$version')"
done

# Operations runbook

This runbook describes the supported operational workflow for the starter. It
does not prescribe Kubernetes, Helm, or a cloud provider; deployment packaging
is a developer or platform-team choice.

## Before deployment

- Build and test the intended profile with the hosted CI workflow.
- Pin the image to a release tag such as `v0.2.0`; do not use `latest` in a
  production deployment.
- Provide a long random `SECRET_KEY` and database credentials through the
  deployment secret store.
- Set `DB_CONNECTION_POOL_SIZE` below the database connection budget after
  accounting for every application replica and other clients.
- Keep `REDIS_ENABLED=false` unless Redis is available and the service was
  built with Redis support.
- Configure `ERROR_TRACKING_PROVIDER` as `none`, `sentry`, or `otlp`. Sentry
  uses `SENTRY_DSN`; OTLP/HTTP uses `OTLP_ENDPOINT`.
  Enable a provider only when it is ready to receive events.

## Local or Compose startup

For a complete development stack:

```sh
./scripts/compose-start.sh
curl --fail http://127.0.0.1:8000/health
curl --fail http://127.0.0.1:8000/ready
```

Stop the stack when finished:

```sh
docker compose down
```

Use `COMPOSE_BUILD=false ./scripts/compose-start.sh` only when the expected
image already exists locally. CI performs the Docker build and integration
validation on Ubuntu; a local C++ toolchain is not required for hosted checks.

## Migrations and seed data

Migrations are an explicit deployment step and must complete before new API
replicas receive traffic:

```sh
./scripts/migrate.sh
./scripts/seed.sh
```

Run migrations once per release using a role permitted to change schema. Never
run them concurrently from every application replica. Take a database backup
before destructive or irreversible migrations, and make migrations backward
compatible when rolling deployments may run old and new binaries together.

## Health diagnosis

- `/health` is liveness: it confirms the process and event loop respond.
- `/ready` is readiness: it confirms the configured database is available.
- `/metrics` exposes aggregate request, response, and 5xx counters.
  It also exposes `observability_events_queued_total`,
  `observability_events_dropped_total`, `observability_batches_sent_total`,
  and `observability_batch_events_total` for exporter queue pressure and
  delivery monitoring. `observability_retries_total`,
  `observability_failures_total`, and `observability_circuit_open_total` show
  retry pressure, exhausted deliveries, and batches rejected while the circuit
  breaker is open.

If `/health` fails, inspect process logs and restart the failed instance. If
`/health` passes but `/ready` fails, check database DNS, credentials, pool
capacity, migrations, and PostgreSQL availability. Redis is an optional cache;
its failure should not stop normal PostgreSQL-backed requests.

Every request includes `X-Request-ID` and W3C `traceparent` response headers.
Use these values to correlate client reports with structured request logs and
error-reporter events.

## Rollback

1. Stop or pause rollout of the new application version.
2. Confirm `/health`, `/ready`, error rate, latency, and database capacity.
3. Roll back to the previous immutable image tag.
4. Do not automatically reverse a migration; use a reviewed corrective
   migration or restore procedure.
5. Record the release, migration versions, symptoms, and recovery time.

## Backups and recovery

The service does not create database backups. The operating environment must
provide scheduled PostgreSQL backups, retention, encryption, restore tests,
and a documented recovery point and recovery time objective. Verify restores
regularly in an isolated environment before relying on them during an incident.

## Rate limiting and shutdown

`RATE_LIMIT_REQUESTS=0` disables the built-in limiter. A positive value enables
a per-client sliding-window limit; health, readiness, and metrics are exempt.
The limiter is process-local, so multi-instance deployments need a gateway or
distributed Redis-backed policy.

Drogon handles `SIGTERM` and `SIGINT` through its graceful `quit()` lifecycle.
Set the deployment termination grace period longer than the expected
in-flight request duration and verify that traffic is removed before process
termination.

## Incident checklist

1. Capture the time window, release tag, request ID, traceparent, endpoint, and
   observed status/latency.
2. Check `/health`, `/ready`, `/metrics`, application logs, PostgreSQL, and
   Redis independently.
3. Protect the database from overload before increasing API replicas.
4. Disable optional integrations only if they affect application control flow;
   observability providers are expected to fail open.
5. Communicate mitigation, owner, and next update time.
6. Follow up with a root-cause review and a tested preventive change.

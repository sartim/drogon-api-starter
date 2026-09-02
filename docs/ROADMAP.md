# Drogon API platform starter roadmap

This roadmap tracks the repository as a production-oriented Drogon starter.
The user/RBAC API is the default batteries-included profile; a minimal profile
is available when a developer wants only the platform foundation.

## Priority key

- **P0 — Now:** required to make the starter dependable for everyday use.
- **P1 — Next:** high-value production capability after P0 is complete.
- **P2 — Later:** valuable extension that depends on the platform foundation.
- **P3 — Future:** optional ecosystem or advanced-scale capability.

## Completed

- [x] **P0** Establish Drogon C++ service, PostgreSQL persistence, JWT/RBAC,
      migrations, seeding, OpenAPI/Swagger, metrics, request IDs, logging,
      unit tests, CI, and multi-stage Docker builds.
- [x] **P0** Add minimal and user-service CMake profiles and presets.
- [x] **P0** Add optional Redis-backed pagination caching with fail-open behavior.
- [x] **P0** Add PostgreSQL and Redis Compose services with an explicit
      one-shot migration service.
- [x] **P0** Add `/health` liveness and `/ready` database readiness endpoints.
- [x] **P0** Add the fresh-project generator with minimal and user-service
      profiles.
- [x] **P0** Add CI smoke tests for Compose startup, migrations, health,
      readiness, documentation, metrics, and protected routes.
- [x] **P0** Add CI integration tests for JWT login, authenticated CRUD, and
      pagination using a disposable database fixture.
- [x] **P0** Automate semantic releases with `vX.Y.Z` tags; `v0.2.0` is
      published.

## P0 — Now: dependable starter workflow

- [ ] Merge the Redis fallback and database readiness-degradation tests
      ([PR #37](https://github.com/sartim/drogon-api-starter/pull/37)).
- [ ] Make a fresh clone runnable with one documented command covering
      configuration, migrations, seeding, Swagger, and health validation.
- [ ] Pin Drogon and third-party dependency versions or commits and document
      the upgrade policy.
- [ ] Introduce vendor-neutral error-reporting and APM interfaces with a safe
      no-op provider; keep Sentry, OpenTelemetry, and other exporters optional.
- [ ] Complete and merge the Dependabot configuration
      ([PR #15](https://github.com/sartim/drogon-api-starter/pull/15)).
- [ ] Merge the macOS metadata protection
      ([PR #18](https://github.com/sartim/drogon-api-starter/pull/18)).

## P1 — Next: portable observability and deployment operations

- [ ] Add provider adapters for OpenTelemetry/OTLP and Sentry without exposing
      vendor SDKs to application or service code.
- [ ] Propagate request ID, trace ID, route, status, version, and bounded
      contextual fields while keeping reporting asynchronous and fail-open.

- [ ] Add Kubernetes Deployment, Service, ConfigMap, Secret example, resource
      requests/limits, probes, and optional Ingress.
- [ ] Add a Kubernetes migration Job with documented rollout ordering.
- [ ] Add Prometheus scrape annotations and production logging/tracing guidance.
- [ ] Add graceful shutdown, connection-pool tuning, timeouts, and rate limits.
- [ ] Add an operational runbook for backups, rollbacks, migrations, and
      incident response.

## P2 — Following: reusable project generation

- [ ] Define a stable generated layout for `app`, `platform`, `examples`,
      `migrations`, `tests`, `deploy`, and `docs`.
- [ ] Keep example-specific migrations, configuration, and tests isolated from
      the platform foundation.
- [ ] Add a generator wrapper around `drogon_ctl` for controllers, filters,
      models, and views.
- [ ] Add golden-output tests for generated minimal and user-service projects.
- [ ] Publish contribution, compatibility, and release-maintenance policies.

## P3 — Future: optional gRPC adapter

- [ ] Add `ENABLE_GRPC=OFF` without adding gRPC dependencies to REST builds.
- [ ] Define versioned protobuf contracts and generate C++ sources at build time.
- [ ] Implement separate gRPC server/client adapters on a dedicated port.
- [ ] Route REST and gRPC adapters through the same application services.
- [ ] Share authentication, request IDs, tracing, metrics, deadlines, and
      cancellation rules.
- [ ] Add gRPC health, integration tests, TLS, message limits, and deployment
      configuration only when enabled.

## Engineering principles

- REST is the default public adapter; gRPC is opt-in for internal or strongly
  typed service communication.
- Keep business logic in application services and adapters thin.
- Prefer asynchronous I/O on high-throughput paths; the current synchronous
  ORM boundary is a deliberate follow-up before extreme-load production use.
- Optional integrations must fail open where safe and never make the baseline
  service require credentials or infrastructure it does not use.
- Every milestone should be independently reviewable through a pull request.

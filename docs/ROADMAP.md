# Drogon API platform starter roadmap

The goal is to make this repository a maintained production template for
serious Drogon services. The current user/RBAC API remains a working example
module; platform capabilities should be reusable by new services.

## Build profiles

The default profile must be a minimal Drogon API that does not require the
example user service, Redis, or gRPC. Developers opt into capabilities as they
need them:

```text
minimal       platform foundation only
user-example  minimal + authentication + users/RBAC + migrations
redis         optional cache integration
grpc          optional gRPC adapter
```

The profiles should be available through `CMakePresets.json` and equivalent
documented CMake options. This keeps the first-run experience small while
preserving the example code as a complete reference implementation.

## Milestone 1: runnable platform baseline

- [x] Drogon, PostgreSQL, JWT authentication, migrations, seeding, OpenAPI,
      Docker, CI, unit tests, metrics, and request IDs
- [x] Shared application/service boundaries for users and roles
- [x] Optional Redis-backed pagination cache with fail-open behavior
- [ ] Add PostgreSQL to the default Docker Compose stack
- [ ] Add `/ready` for PostgreSQL readiness and optional Redis readiness
- [ ] Provide one-command startup, migration, seeding, Swagger, and health
      validation from a fresh clone
- [ ] Make the minimal profile the default and move user/RBAC code behind an
      opt-in example profile
- [ ] Add `CMakePresets.json` for minimal, user-example, Redis, and CI builds

## Milestone 2: reliable delivery and integration testing

- [ ] Add PostgreSQL and Redis service containers to GitHub Actions
- [ ] Add integration tests for migrations, authentication, CRUD, pagination,
      cache fallback, and readiness failures
- [ ] Keep migrations as an explicit deployment job, never run them from every
      application replica
- [ ] Pin external dependency versions or commits and document upgrade policy
- [ ] Complete Dependabot and semantic release configuration

## Milestone 3: deployment-ready operations

- [ ] Add Kubernetes Deployment, Service, ConfigMap, Secret example, probes,
      resource requests/limits, and optional Ingress
- [ ] Add a Kubernetes migration Job and documented rollout ordering
- [ ] Add Prometheus scrape annotations and production logging/tracing guidance
- [ ] Add graceful shutdown, connection pool tuning, timeouts, and rate limits
- [ ] Add an operational runbook for backup, rollback, migrations, and incidents

## Milestone 4: optional gRPC adapter

- [ ] Add a CMake option such as `ENABLE_GRPC=OFF` and keep REST builds free of
      gRPC dependencies by default
- [ ] Define versioned protobuf contracts and generate C++ sources during build
- [ ] Implement separate gRPC server/client adapters on a dedicated port
- [ ] Route both REST and gRPC adapters through the same application services
- [ ] Share authentication, request IDs, tracing, metrics, deadlines, and
      cancellation rules across adapters
- [ ] Add gRPC health, integration tests, TLS, message limits, and deployment
      configuration only when the adapter is enabled

## Milestone 5: reusable project generation

- [ ] Define a stable template layout for `app`, `platform`, `examples`,
      `migrations`, `tests`, `deploy`, and `docs`
- [ ] Keep example-specific migrations, configuration, and tests isolated from
      the platform foundation
- [ ] Add a generator wrapper around `drogon_ctl` for project, controller,
      filter, model, and view scaffolding
- [ ] Provide a minimal generated example separate from the user/RBAC example
- [ ] Publish contribution, compatibility, and release-maintenance policies

## Engineering principles

- REST is the default public adapter; gRPC is opt-in for internal or strongly
  typed service communication.
- Keep business logic in application services and adapters thin.
- Prefer asynchronous I/O on high-throughput paths; the current synchronous
  ORM boundary is a deliberate follow-up before extreme-load production use.
- Optional integrations must fail open where safe and must never make the
  baseline service require credentials or infrastructure it does not use.
- Every milestone should be independently reviewable through a pull request.

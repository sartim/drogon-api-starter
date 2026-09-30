# Engineering Roadmap

This roadmap tracks the work required to evolve the Drogon user service into a
scalable, reusable HTTP and gRPC service. Each milestone should be delivered
as a focused pull request with local validation and CI coverage.

## Completed

- [x] Native macOS and Ubuntu 24.04 setup scripts
- [x] Docker build and GitHub Actions validation
- [x] Unit-test execution through CTest
- [x] Database migrations and repeatable seed scripts
- [x] OpenAPI specification and Swagger UI
- [x] Formatting and clang-tidy scripts
- [x] Semantic versioning and automated release workflow
- [x] Dependabot configuration
- [x] Request IDs, request logging, and Prometheus-compatible metrics
- [x] Optional fail-open Sentry configuration
- [x] Single `.env` source with in-memory Drogon configuration

## Milestone 1: Configuration and application boundaries

- [ ] Add a typed `AppConfig` model for server, database, security, gRPC, and
      observability settings.
- [ ] Validate required configuration at startup with actionable errors.
- [ ] Support environment variables as production overrides for `.env`.
- [ ] Remove configuration and route-registration responsibilities from
      `main.cc`.
- [ ] Add dependency injection seams for services and repositories.

Acceptance criteria:

- Startup has one configuration path and no secret-bearing generated files.
- Unit tests can construct application services without starting Drogon.
- Invalid configuration fails before listeners are opened.

## Milestone 2: Domain and service-layer structure

- [ ] Organize code by feature: `auth`, `users`, and `roles`.
- [ ] Introduce application services between transports and repositories.
- [ ] Keep controllers focused on transport parsing and response mapping.
- [ ] Define repository interfaces for database access.
- [ ] Centralize validation, authorization, pagination, and error mapping.
- [ ] Remove duplicated CRUD and response-building logic where behavior is
      genuinely shared.

Acceptance criteria:

- HTTP handlers do not contain business rules or SQL details.
- Shared use cases are callable from both HTTP and gRPC adapters.
- Repeated behavior has one tested implementation.

## Milestone 3: HTTP standardization

- [ ] Split route registration into feature-specific modules.
- [ ] Add a consistent versioned response and error envelope.
- [ ] Add request-size, timeout, and input-validation policies.
- [ ] Add pagination and filtering conventions to collection endpoints.
- [ ] Keep OpenAPI documentation synchronized with the implementation.

Acceptance criteria:

- Existing REST behavior remains backward compatible unless explicitly
  versioned.
- Every error includes a stable error code and request ID.
- API contract tests cover success and failure responses.

## Milestone 4: gRPC foundation

- [ ] Add protobuf contracts under `proto/`.
- [ ] Generate C++ protobuf and gRPC sources through CMake.
- [ ] Add an optional gRPC build controlled by configuration.
- [ ] Run the gRPC server alongside Drogon on a separate configurable port.
- [ ] Add gRPC health checking and development-only reflection.
- [ ] Map shared application-service errors to gRPC status codes.

Target layout:

```text
src/
  features/
    auth/
    users/
    roles/
  transports/
    http/
    grpc/
  infrastructure/
    database/
    security/
  config/
  observability/
proto/
test/
  unit/
  integration/
```

Acceptance criteria:

- HTTP and gRPC use the same application services and authorization rules.
- gRPC can be disabled without affecting the REST service build or runtime.
- A generated gRPC client can call health and one user-service operation.

## Milestone 5: Security and identity

- [ ] Centralize JWT verification and authorization policy evaluation.
- [ ] Propagate identity and request metadata from HTTP/gRPC transports.
- [ ] Validate and bound incoming request IDs and metadata.
- [ ] Redact credentials, tokens, and sensitive fields from logs.
- [ ] Protect metrics and reflection endpoints in production.

Acceptance criteria:

- Security rules are transport-independent and tested once at the service
  layer.
- Sensitive values never appear in application logs or generated artifacts.

## Milestone 6: Production observability

- [ ] Add W3C `traceparent` propagation.
- [ ] Add request duration and database duration metrics.
- [ ] Add real Sentry reporting behind an optional build/configuration adapter.
- [ ] Add health/readiness distinction for database dependencies.
- [ ] Add configurable log levels and structured log output.

Acceptance criteria:

- Missing telemetry credentials never prevent startup.
- HTTP and gRPC requests can be correlated across logs and traces.
- Metrics avoid unbounded labels such as user IDs or unrestricted URLs.

## Milestone 7: Test and quality maturity

- [ ] Mirror production features in `test/unit` and `test/integration`.
- [ ] Add PostgreSQL integration tests for repositories and migrations.
- [ ] Add HTTP and gRPC contract tests.
- [ ] Add compiler warnings, clang-format, and clang-tidy CI gates.
- [ ] Add AddressSanitizer and UndefinedBehaviorSanitizer jobs.
- [ ] Add OpenAPI and protobuf compatibility checks.

Acceptance criteria:

- Every pull request runs build, unit tests, static analysis, and relevant
  integration tests.
- The service can be tested without Docker on macOS and Ubuntu 24.04.

## Milestone 8: Delivery and operations

- [ ] Publish versioned multi-platform images.
- [ ] Add migration execution as an explicit deployment step.
- [ ] Add release notes and rollback guidance.
- [ ] Add resource limits, graceful shutdown, and connection-pool tuning.
- [ ] Load-test HTTP and gRPC paths and record capacity baselines.

Acceptance criteria:

- Releases are reproducible and traceable to a Git tag in `vX.Y.Z` format.
- Deployments can migrate, verify readiness, and roll back safely.
- Capacity and failure-mode behavior are documented.

## Working rules

- One milestone or coherent slice per pull request.
- Preserve API compatibility unless a versioned change is intentional.
- Prefer composition and explicit interfaces over inheritance-heavy frameworks.
- Keep domain logic independent of Drogon, gRPC, and database implementation
  details.
- Update this roadmap in the same pull request when a milestone changes state.

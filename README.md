# Drogon API platform starter

[![Language](https://img.shields.io/badge/language-cpp-green.svg)](https://github.com/sartim/drogon-api-starter)
[![Build Status](https://github.com/sartim/drogon-api-starter/workflows/build/badge.svg)](https://github.com/sartim/drogon-api-starter)

Production-oriented Drogon API starter with PostgreSQL, optional Redis,
authentication, migrations, OpenAPI, Docker Compose, observability, CI, and a
user/RBAC example module. See the [platform roadmap](docs/ROADMAP.md).

The REST API is the default. Optional gRPC adapters will share the same
application services and can be enabled by developers who need internal,
strongly typed service communication; gRPC will not be a baseline dependency.

## Generate a fresh service

Use the generator when starting a new project instead of copying this
repository directly. The destination must be new or empty:

    $ ./scripts/drogon-starter init payments-api ../payments-api

The destination can also be supplied explicitly with `--path`, which is useful
when the project name and filesystem location are far apart:

    $ ./scripts/drogon-starter init payments-api --path /tmp/payments-api

By default this generates the maintained user-service profile with JWT
authentication, users, roles, RBAC, migrations, tests, and the platform
foundation already present in this repository:

    $ ./scripts/drogon-starter init users-api ../users-api

For a clean Drogon foundation without the user-service batteries, select the
minimal profile:

    $ ./scripts/drogon-starter init payments-api ../payments-api --profile minimal

Both profiles create stable extension points for `platform`, `examples`,
`migrations`, and `deploy` in addition to their profile-specific source,
tests, and documentation. These directories start empty so teams can add
capabilities without changing the generator contract. CI runs a generator
contract test for both profiles and compares the minimal output with its
checked-in golden templates.

You can upgrade a generated minimal project to the maintained user-service
batteries explicitly. This replaces the generated scaffold files, so review
the resulting Git diff before committing:

    $ ./scripts/drogon-starter enable user-service ../payments-api --force

## Generate Drogon components

After installing `drogon_ctl` (the native setup scripts build it automatically),
use the wrapper to keep generated components in
the starter layout. It delegates generation to Drogon and does not require any
project database or Docker services:

```sh
./scripts/drogon-generate controller UserController --output app/controllers
./scripts/drogon-generate filter AuthFilter --output app/filters
./scripts/drogon-generate model schemas/users.json --output models
```

Set `DROGON_CTL=/path/to/drogon_ctl` when the executable is not on `PATH`. The
wrapper also supports `plugin` and `view`; use `--` to pass additional native
`drogon_ctl` flags.

## Supported starter profiles

The repository provides batteries-included profiles without forcing every
developer to adopt every feature:

- `minimal`: Drogon platform foundation, configuration, health, metrics,
  documentation, and platform tests
- `user-service`: the minimal profile plus JWT authentication, users, roles,
  RBAC, database models, migrations, and service tests

The minimal profile compiles only the reusable platform components. User/RBAC
controllers, filters, models, tables, schemas, and services are added only
when `ENABLE_USER_SERVICE=ON` (the `user-service` preset).

Start with the minimal profile:

    $ cmake --preset minimal
    $ cmake --build --preset minimal
    $ ctest --preset minimal

Or use the complete user-service profile:

    $ cmake --preset user-service
    $ cmake --build --preset user-service
    $ ctest --preset user-service

Redis and gRPC are separate optional capabilities and will be added to these
profiles without becoming mandatory dependencies of the minimal build.

This repository follows a batteries-included profile model. The default
`minimal` profile provides the Drogon platform foundation; the optional
`user-service` profile adds JWT authentication, users, roles, RBAC, and their
tests as a complete supported starting point. Developers can begin with the
minimal profile or select the user-service batteries when they need them.

```sh
# Minimal Drogon API
cmake --preset minimal
cmake --build --preset minimal
ctest --preset minimal

# Drogon API with the user-service batteries
cmake --preset user-service
cmake --build --preset user-service
ctest --preset user-service
```

## Requirements

* [Drogon](https://github.com/drogonframework/drogon)
* [PostgreSQL](https://www.postgresql.org)
* [JWT-CPP](https://github.com/Thalhammer/jwt-cpp)
* [Bcrypt](https://git@github.com:hilch/Bcrypt.cpp.git)
* [OpenSSL](https://github.com/openssl/openssl.git)

## Hosted documentation

The Markdown documentation is built with MkDocs Material and published by GitHub Actions to
[GitHub Pages](https://sartim.github.io/drogon-api-starter/). The workflow
uses only the documentation toolchain; it does not install or build Drogon,
PostgreSQL, Redis, or the C++ service locally. Enable GitHub Pages in the
repository settings with **GitHub Actions** as the source before the first
deployment.

## Local development setup

List endpoints support page-based pagination: use `page` (default `1`) and
`page_size` (default `25`, maximum `100`) on `/api/v1/user` and
`/api/v1/role`. Responses include `results`, `page`, `page_size`, `total`, and
`has_next`.

Redis caching is optional and disabled by default. Set `REDIS_ENABLED=true`
when Drogon was built with hiredis and Redis is reachable. List responses are
cached for 30 seconds; writes invalidate the resource cache. If Redis is
unavailable, requests continue using PostgreSQL.

Connection-pool and timeout settings are configurable through `.env`:
`DB_CONNECTION_POOL_SIZE`, `DB_QUERY_TIMEOUT_SECONDS`,
`REDIS_CONNECTION_POOL_SIZE`, `REDIS_COMMAND_TIMEOUT_SECONDS`, and
`HTTP_IDLE_CONNECTION_TIMEOUT_SECONDS`. The defaults are conservative for
local development; production values should be sized against database limits,
worker count, and expected concurrency.

Set `RATE_LIMIT_REQUESTS` to a positive value to enable the built-in sliding
window limiter per client address; it is disabled by default (`0`). Health,
readiness, and metrics endpoints are excluded so orchestration can continue to
operate. This limiter is process-local and should be replaced or fronted by a
distributed gateway/Redis policy when running multiple replicas.

Drogon handles `SIGTERM` and `SIGINT` by stopping the application event loop
through its graceful `quit()` lifecycle. Deployments should still provide a
termination grace period longer than the expected in-flight request duration.

To run a complete local stack with Redis:

```sh
./scripts/compose-start.sh
```

The bootstrap command creates `.env` when needed, starts PostgreSQL and Redis,
runs migrations, loads reference seed data, starts the user-service profile,
and validates both liveness and readiness. It builds locally by default. When
`user_service:latest` is already available, use `COMPOSE_BUILD=false` to skip
the build.

For native development, use `./scripts/setup_local.sh` on macOS or
`./scripts/setup_ubuntu24.sh` on Ubuntu 24.04, then run:

```sh
ctest --test-dir build --output-on-failure
```

The setup scripts install hiredis and build Drogon with Redis support, while
leaving `REDIS_ENABLED=false` in `.env` so PostgreSQL-only development remains
frictionless. Enable it after starting Redis.

On macOS, the repository includes a bootstrap script for a fresh clone. It
installs the Homebrew dependencies, fetches Bcrypt.cpp, starts a local
PostgreSQL service, creates a development database, creates `.env` only when it
does not already exist, configures the project, builds the service and test
binary, and runs the unit tests:

    $ ./scripts/setup_local.sh

The script requires [Homebrew](https://brew.sh/). It does not overwrite an
existing `.env`. To run the service itself after setup:

    $ cmake --build build --parallel
    $ ./build/drogon_user_service --action=run-server

CLion can use the same checkout after running the setup script. Open the
repository as a CMake project and select the `Debug` profile; CMake discovers
the Drogon and jwt-cpp installations under `.local` automatically. If using a
manual CLion profile, the equivalent configuration is:

    Generator: Ninja
    CMake options: -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

To create the application tables in the local database:

    $ ./build/drogon_user_service --action=create-tables

To run tests after the initial setup:

    $ ctest --test-dir build --output-on-failure

To rebuild and run the complete local unit-test suite from a clean checkout:

    $ ./scripts/setup_local.sh

To run only the test binary with verbose DrogonTest output:

    $ ./build/test/drogon_user_service_test

## Database migrations and seeding

Database structure is managed with ordered SQL migrations in
`db/migrations/`. Each migration is applied once and recorded in the
`schema_migrations` table. Migrations run with `ON_ERROR_STOP` and each file is
applied in a transaction, so a failed migration is not recorded as complete:

    $ ./scripts/migrate.sh

Reference data is kept separately in `db/seeds/` and is safe to run repeatedly
because inserts use conflict handling:

    $ ./scripts/seed.sh

Run migrations before seeding. Never edit an already-applied migration; add a
new numbered migration instead. Production migrations should run as a
deployment step using a database role that can migrate but is separate from
the application runtime role.

The old `--action=create-tables` command is retained for compatibility but new
environments should use `./scripts/migrate.sh`.

## API documentation

The OpenAPI specification is in `docs/openapi.yaml` and Swagger UI is served
by the application at:

    http://localhost:8000/docs

The raw specification is available at:

    http://localhost:8000/openapi.yaml

The Swagger page loads its UI assets from the public Swagger UI CDN. For an
offline or production deployment, vendor those assets or serve Swagger UI
behind the deployment's static asset pipeline.

## Observability

The service exposes a Prometheus-compatible metrics endpoint at
`GET /metrics`. It records aggregate request, response, and HTTP 5xx counters
without adding path or user labels that could create high-cardinality metrics.

Each request receives an `X-Request-ID` response header. An incoming
`X-Request-ID` is preserved; otherwise the service generates one. Request
start/completion logs include the ID, method, path, and response status, which
provides a lightweight trace across application logs.

The service also accepts and returns the standard W3C `traceparent` header.
This propagates distributed trace context without requiring an APM SDK in the
baseline build; OpenTelemetry or vendor adapters can consume it later.

Error tracking is provider-neutral and fail-open. The application depends on
the `ErrorReporter` interface, while provider SDKs remain optional adapters.
Optional integrations can register an adapter with
`registerErrorReporterProvider(...)`; the core service falls back to the
no-op reporter when an adapter is absent or fails to initialize. This keeps
Sentry, OpenTelemetry/OTLP, and other APM SDKs outside application code.
Set `ERROR_TRACKING_PROVIDER=none` for the no-op default. Sentry, OpenTelemetry,
Datadog, and other adapters can be added without changing application or
service code. `SENTRY_DSN` is retained as an optional Sentry adapter setting:

    $ export SENTRY_DSN="https://examplePublicKey@o0.ingest.sentry.io/0"

Alternatively, add the value to a local `.env` copied from `.env.example`.
Leave `SENTRY_DSN` empty when Sentry is not available.

For exception handling inside a controller or service, pass the Drogon request
to `observability::captureException(error, request)`. The platform enriches
the event with the request ID, W3C `traceparent`, HTTP method, and path while
allowing application context fields to be added or override defaults.

The built-in OTLP adapter uses Drogon’s asynchronous HTTP client and requires
only an endpoint; no OpenTelemetry SDK is required. Set
`ERROR_TRACKING_PROVIDER=otlp` and `OTLP_ENDPOINT` to an OTLP/HTTP logs
endpoint, for example:

    $ export OTLP_ENDPOINT="http://localhost:4318/v1/logs"

Use `OBSERVABILITY_TIMEOUT_SECONDS` to bound delivery attempts. Invalid
configuration and failed delivery fall back to no-op behavior and never block
request handling.

Events are buffered briefly and delivered in bounded batches. Tune
`OBSERVABILITY_BATCH_SIZE` and `OBSERVABILITY_BATCH_DELAY_SECONDS` for the
collector and traffic profile; queued events are best effort and may be
dropped during shutdown or when the queue is full.

Keep provider credentials in the deployment secret store rather than committing
them to `.env` or source control. Unsupported providers currently fall back to
the no-op reporter and do not prevent startup; provider adapters will be added
behind separate optional build dependencies.

## Formatting and linting

Install the tools with Homebrew on macOS:

    $ brew install clang-format llvm

On Ubuntu 24.04:

    $ sudo apt-get install clang-format clang-tidy

Format tracked C++ files in place:

    $ ./scripts/format.sh

Check formatting without changing files:

    $ ./scripts/format_check.sh

Run clang-tidy after configuring the project with compile commands enabled:

    $ ./scripts/lint.sh

The setup scripts enable `compile_commands.json` automatically. Formatting and
linting should be run before opening a pull request; CI remains responsible
for the Docker build, unit tests, and health-check integration test.

The unit tests do not require PostgreSQL or a running API server. The server
is started separately with `./build/drogon_user_service --action=run-server`.
At startup, the service reads `.env`, validates it into a typed configuration,
and builds the Drogon configuration in memory. Environment variables override
matching `.env` values, which allows production deployments to use a secret
manager without writing a `config.json` file.

The liveness endpoint is implemented by the application at
`GET /health` and returns `{"status":"up"}` without requiring a database
connection. The Docker healthcheck calls this endpoint.

On Linux, install Drogon, PostgreSQL development headers, OpenSSL, jwt-cpp,
libpqxx, JsonCpp, CMake, and a C++17 compiler using your distribution's
package manager. Then clone Bcrypt.cpp into the project root and use the same
`cmake`, `cmake --build`, and `ctest` commands. Docker remains available for a
fully isolated Linux build.

For Ubuntu 24.04, the repository provides an automated native setup:

    $ ./scripts/setup_ubuntu24.sh

It installs the required apt packages, builds Drogon with PostgreSQL support,
builds jwt-cpp and Bcrypt.cpp from their repositories, then runs the local
unit tests. It does not start the server; start it with:

    $ ./build/drogon_user_service --action=run-server

## Create .env file

    SECRET_KEY={SECRET_KEY}
    DB_NAME={DB_NAME}
    DB_USER={DB_USER}
    DB_PASSWORD={DB_PASSWORD}

## Setup bcrypt

On the project root:

    $ git clone https://github.com/hilch/Bcrypt.cpp.git

## Install jwt-cpp

    $ cd build
    $ cmake ..
    $ make
    $ sudo make install
    

## Install OpenSSL

    $ cd openssl
    $ ./config shared no-ssl2
    $ make
    $ make install


## Running server

    $ cd build
    $ cmake ..
    $ make
    $ ./drogoncore_user_service --action=run-server

## Running with docker
    
    $ docker compose up --build

The Dockerfile uses a multi-stage build. The builder contains compilers and
development dependencies; the final runtime image contains only the service,
runtime libraries, and API documentation. Configuration is supplied at
container startup, so database credentials are not stored in image layers.
CI runs the unit tests in the builder stage and uses the slim runtime stage
only for service startup and endpoint checks.

Docker source dependencies are pinned to reviewed tags and commits for
reproducible builds. Upgrade them deliberately in `Dockerfile`, run the CI
matrix, and review the resulting dependency changes. Dependabot continues to
update base images, Docker dependencies, and GitHub Actions where supported.

## CI integration smoke test

GitHub Actions runs the user-service image with PostgreSQL and Redis using the
Compose stack. The integration smoke test validates migrations through the
one-shot migration service, database readiness, documentation, metrics, and
that protected user and role endpoints reject unauthenticated requests.

To run the endpoint checks against an already-running stack:

    $ ./scripts/integration_smoke.sh

The CI workflow seeds a disposable test user and then exercises JWT login,
authenticated create/list/get/update/delete operations, and paginated listing.
It also stops Redis to verify cache fail-open behavior, then stops PostgreSQL
to verify `/ready` returns HTTP 503 when the database is unavailable.
The script does not build images or start containers, so local Docker remains
optional; it expects an already-running user-service stack and `jq`.

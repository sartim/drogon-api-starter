# User Service

[![Language](https://img.shields.io/badge/language-cpp-green.svg)](https://github.com/sartim/drogon_user_service)
[![Build Status](https://github.com/sartim/drogon_user_service/workflows/build/badge.svg)](https://github.com/sartim/drogon_user_service)

User service running on Drogon Framework which handles RBAC management. Make
sure to create a `.env` file from `.env.example`.

## Requirements

* [Drogon](https://github.com/drogonframework/drogon)
* [PostgreSQL](https://www.postgresql.org)
* [JWT-CPP](https://github.com/Thalhammer/jwt-cpp)
* [Bcrypt](https://git@github.com:hilch/Bcrypt.cpp.git)
* [OpenSSL](https://github.com/openssl/openssl.git)

## Local development setup

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

Sentry is optional. If `SENTRY_DSN` is not set, the service logs that Sentry is
disabled and continues normally. A DSN is detected for future error-tracking
integration, but this build does not require or link the Sentry SDK:

    $ export SENTRY_DSN="https://examplePublicKey@o0.ingest.sentry.io/0"

Alternatively, add the value to a local `.env` copied from `.env.example`.
Leave `SENTRY_DSN` empty when Sentry is not available.

Keep the DSN in the deployment secret store rather than committing it to
`.env` or source control. The current error-tracking hook is intentionally
fail-open; integrating the Sentry SDK can be added once the deployment
environment provides the required credentials and transport policy.

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
At startup, the service reads `.env`, builds the Drogon configuration in
memory, and does not create or require a `config.json` file.

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

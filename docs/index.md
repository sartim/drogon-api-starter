# Drogon API platform starter

A production-oriented C++ starter for building Drogon APIs with a clear path
from a minimal service to a batteries-included user/RBAC API.

## Start a new service

The generator creates a fresh project in a specified empty directory:

```sh
./scripts/drogon-starter init payments-api ../payments-api --profile minimal
./scripts/drogon-starter init users-api ../users-api --profile user-service
# Equivalent explicit path form:
./scripts/drogon-starter init payments-api --path ../payments-api --profile minimal
# Upgrade a generated minimal project:
./scripts/drogon-starter enable user-service ../payments-api --force
```

The `minimal` profile provides the platform foundation. The `user-service`
profile adds authentication, users, roles, RBAC, migrations, and tests.

## Documentation

- [Engineering roadmap](ROADMAP.md)
- [Operations runbook](OPERATIONS.md)
- [OpenAPI specification](openapi.yaml)
- [Swagger UI source](swagger.html)
- [Repository README](https://github.com/sartim/drogon-api-starter#readme)

## Platform capabilities

- Drogon REST API with optional gRPC adapter support planned separately
- PostgreSQL migrations and seed data
- Optional Redis caching with fail-open behavior
- JWT authentication and RBAC batteries
- Liveness, readiness, metrics, structured logging, and trace propagation
- Docker Compose, Kubernetes manifests, and hosted CI validation

## Local development

Native setup scripts are available for macOS and Ubuntu 24.04. Docker builds
and integration validation run in GitHub Actions, so documentation publishing
does not require the C++ toolchain or service dependencies on the documentation
author’s machine.

See the [local development section in the README](https://github.com/sartim/drogon-api-starter#local-development-setup)
for the setup commands.

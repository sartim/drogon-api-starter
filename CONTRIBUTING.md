# Contributing

## Commit messages

Use Conventional Commits so release automation can determine the next
semantic version:

    fix: reject malformed JWT tokens
    feat: add role listing endpoint
    docs: improve Ubuntu setup

Use `!` or a `BREAKING CHANGE:` footer for an incompatible API or behavior
change. These commits produce a major release. Normal `fix` commits produce a
patch release, while `feat` commits produce a minor release.

## Releases

Release Please creates a release pull request after changes reach `main`.
Review and merge that pull request to create a GitHub Release and a tag in the
`vX.Y.Z` format, for example `v0.2.0`.

The container publishing workflow runs only for those version tags. It builds
the image, runs CTest, verifies `GET /health`, and then publishes the image to
GHCR.

## Dependency updates

Dependabot checks GitHub Actions and Docker dependencies weekly and opens
pull requests for updates. C++ dependencies are currently built from source
repositories, so they should be upgraded deliberately in a normal pull
request and validated with the native and Docker build workflows.

## Generator and template maintenance

The generator is a supported interface. Keep the `minimal` and
`user-service` profiles independently usable, and preserve the generated
extension points (`platform`, `examples`, `migrations`, and `deploy`). Run the
generator contract test after changing templates or `scripts/drogon-starter`:

    $ bash test/generator_test.sh

The test compares minimal output with its checked-in templates and verifies the
user-service profile has the expected files, substitutions, and extension
points. Changes that intentionally alter generated output should explain the
migration impact in the pull request.

## Release maintenance checklist

Before merging a release-affecting change:

1. Use a Conventional Commit and update documentation when behavior changes.
2. Run the native tests and generator contract test when available.
3. Let hosted CI validate the Docker profiles and integration stack.
4. Review the Release Please pull request; merge it to create the `vX.Y.Z` tag.
5. Confirm the tagged image workflow completes before announcing the release.

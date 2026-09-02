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

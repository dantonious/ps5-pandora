# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed

- **The implementation language is now C++20.** The project was scaffolded in
  Rust and rewritten before any release. The PS5 toolchain, the SDK and every
  payload that runs on the console today are C/C++ (`prospero-clang`), so C++
  removes a variable instead of adding one. The Rust history is preserved in
  git; `NOTICE` records that no code was carried over from another project.

### Added

- CMake build producing `pandora-core`, `pandora-http-core`, `pandora-http` and
  `pandora-cli`, with no external dependencies.
- `pandora-core`: title ID validation, `contentVersion` parsing and ordering,
  `sce_sys/param.json` model, configuration, logging, SHA-256 (verified against
  the published vectors) and a deterministic ZIP writer.
- `pandora-http`: an HTTP/1.1 server and API (`/api/health`, `/api/version`,
  `/api/fs/list`) with the dashboard compiled into the binary.
- `pandora-cli`: `title-id check`, `param show|validate|bump`, `digest`,
  `package` and `doctor` commands.
- Catalog-ready `<TITLEID>.zip` packaging, with the archive rooted at the title
  folder and the SHA-256 printed for the release notes.
- A `Platform` interface with a host implementation and a console
  implementation (`src/platform_console.cpp`) that is compiled only for hardware
  and is **unvalidated**.
- 9 test suites (190 assertions) covering version handling, JSON, metadata,
  configuration, SHA-256, ZIP, the platform layer and HTTP routing, plus a
  packaging end-to-end test.
- Warnings as errors: `-Wall -Wextra -Wpedantic -Wshadow -Wconversion
  -Wsign-conversion -Wnon-virtual-dtor -Wold-style-cast -Werror`.
- Governance: CONTRIBUTING, GOVERNANCE, SECURITY, CODE_OF_CONDUCT, issue and
  pull request templates, CODEOWNERS, and CI.

[Unreleased]: https://github.com/dantonious/ps5-pandora/commits/main

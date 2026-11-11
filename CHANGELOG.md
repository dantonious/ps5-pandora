# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Initial workspace: `pandora-core`, `pandora-http`, `pandora-cli`.
- `pandora-core`: title ID validation, `contentVersion` parsing and ordering,
  `sce_sys/param.json` model, configuration, and a platform abstraction with a
  host implementation for development.
- `pandora-http`: HTTP API skeleton (`/api/health`, `/api/version`,
  `/api/fs/list`) serving an embedded UI.
- `pandora-cli`: `title-id`, `param`, `digest`, `package`, `doctor` and `send`
  commands.
- Catalog-ready `<TITLEID>.zip` packaging (`pandora package`).
- CI: `cargo fmt --check`, `cargo clippy -D warnings`, `cargo test`.

[Unreleased]: https://github.com/dantonious/ps5-pandora/commits/main

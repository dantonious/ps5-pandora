# ps5-pandora

**An all-in-one manager for your jailbroken PlayStation 5 — written in Rust.**

`ps5-pandora` is an independent, from-scratch implementation of the
"one payload, one dashboard" idea: a single payload runs on the console, serves
its entire interface over HTTP, and lets you manage the system from any browser
on the same network. No companion desktop application, no separate web server.

> **Status: pre-alpha.** This repository is a validated skeleton, not a usable
> build yet. [docs/ROADMAP.md](docs/ROADMAP.md) states exactly what runs today
> and what does not. Do not install it on a console you care about.

## Why Rust

- **One static ELF.** The same deployment model payloads already use: send one
  file, get the whole app.
- **`no_std`-friendly.** The core does not depend on a runtime being ported to
  the console before it can run there.
- **Bounded `unsafe`.** All FFI lives in one thin module; everything else is
  ordinary safe Rust.
- **It runs with high privilege on a device that is hard to debug.** Memory
  safety is not a luxury here.

## Relationship to Prospero Manager

`ps5-pandora` is **not a fork and shares no code** with
[Prospero Manager](https://github.com/notmaj0r/ProsperoMgr). It is an
independent implementation that was *inspired by* the same idea. Credit for
that idea, and for proving it worth building, belongs to the Prospero Manager
authors — see [NOTICE](NOTICE).

If you want a mature product today, use Prospero Manager. If you want to build
a Rust-native one, you are in the right place.

## What works today

- A workspace that builds and tests on a normal Linux/macOS/Windows machine,
  with no console and no SDK required.
- A platform abstraction (`pandora-core::platform`) with a working **host**
  implementation, so console logic can be written and unit-tested off-device.
- `pandora-core`: title ID validation, `contentVersion` parsing/ordering,
  `sce_sys/param.json` handling and configuration.
- `pandora-http`: an HTTP API skeleton (`/api/health`, `/api/version`,
  `/api/fs/list`) with an embedded UI page.
- `pandora-cli`: `title-id`, `param`, `digest`, `package`, `doctor`, `send`.
  `pandora package` produces the catalog-ready `<TITLEID>.zip`.
- CI gates: `cargo fmt --check`, `cargo clippy -- -D warnings`, `cargo test`.

## What does not work yet

- Everything console-specific. The `platform::ps5` module is a
  **placeholder that has never been run on hardware**.
- Storage operations, PKG installation, payload launching, save data, thermal
  control, process management.
- The real frontend. The embedded page is a placeholder, not the React UI.

## Build and run

Host build (no SDK needed):

```sh
cargo build --workspace
cargo test  --workspace
cargo run -p pandora-http            # serves http://127.0.0.1:7070
```

Package a title (this works today; it archives `pkg/`):

```sh
make package                          # -> dist/PPSA77010.zip
```

Console builds are **not wired up yet** — the target spec and SDK integration
are milestone M0 in [docs/ROADMAP.md](docs/ROADMAP.md). [docs/BUILDING.md](docs/BUILDING.md)
explains what exists and what does not, rather than pretending.

## Try the CLI

```sh
cargo run -p pandora-cli -- title-id check PPSA77010
cargo run -p pandora-cli -- param validate pkg/sce_sys/param.json
cargo run -p pandora-cli -- digest dist/PPSA77010.zip
cargo run -p pandora-cli -- doctor
```

## Repository layout

```
crates/
  pandora-core/   Domain types, platform abstraction, title metadata. no_std-friendly.
  pandora-http/   Axum server + embedded UI. The console-side payload grows here.
  pandora-cli/    Host-side tooling: validation, packaging, digest, payload sending.
ui/               Frontend sources (placeholder page today).
pkg/              Native title sources: sce_sys/param.json, icon0.png.
docs/             Architecture, building, title IDs, releasing, roadmap.
```

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the design and the
reasoning behind the crate split.

## Contributing

Contributions are welcome — code, documentation, translations, and hardware
testing reports especially. Everything you need is in
[CONTRIBUTING.md](CONTRIBUTING.md).

Start here:

- New to the project? Look for issues labelled **`good first issue`**.
- Read [CONTRIBUTING.md](CONTRIBUTING.md) before your first pull request.
- Contributions are accepted under the [DCO](https://developercertificate.org/);
  sign off your commits with `git commit -s`.
- By participating you agree to the
  [Code of Conduct](CODE_OF_CONDUCT.md).
- Found a security problem? Follow [SECURITY.md](SECURITY.md) — do not open a
  public issue.

## Licence

GNU General Public License v3.0 or later — see [LICENSE](LICENSE).

`ps5-pandora` is free software. It is intended for **already-jailbroken**
consoles, and it is not a jailbreak, a piracy tool, or a way to obtain content
you do not own.

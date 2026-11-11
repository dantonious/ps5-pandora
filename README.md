# ps5-pandora

**An all-in-one manager for your jailbroken PlayStation 5 — written in C++20.**

`ps5-pandora` is an independent, from-scratch implementation of the
"one payload, one dashboard" idea: a single program runs on the console, serves
its entire interface over HTTP, and lets you manage the system from the console's
own browser. No companion desktop application, no separate web server.

> **Status: pre-alpha.** This repository is a validated skeleton, not a usable
> build yet. [docs/ROADMAP.md](docs/ROADMAP.md) states exactly what runs today
> and what does not. Do not install it on a console you care about.

## Why C++

The PS5 homebrew ecosystem is C and C++. The PS5 Payload SDK ships
`prospero-clang`/`prospero-clang++`, and the projects this one learns from —
payload loaders, kernel patches, existing managers — are all built with that same
toolchain. Choosing C++ means:

- **The proven path.** Every payload that runs on a PS5 today was compiled this
  way. There is no new runtime to port, no target triple to invent.
- **No dependencies to vendor.** This repository links nothing but the C++
  standard library. There is no JSON library, no HTTP library and no compression
  library to add to the build, patch on the console, or audit — see
  [NOTICE](NOTICE).
- **Allocation and layout are visible.** On a device that is hard to debug and
  runs this code with high privilege, predictable behaviour is worth more than
  convenience.
- **The FFI boundary is explicit.** Every libkernel call lives in one file,
  [src/platform_console.cpp](src/platform_console.cpp), and nowhere else.

## Relationship to Prospero Manager

`ps5-pandora` is **not a fork and shares no code** with
[Prospero Manager](https://github.com/notmaj0r/ProsperoMgr). It is an
independent implementation that was *inspired by* the same idea. Credit for
that idea, and for proving it worth building, belongs to the Prospero Manager
authors — see [NOTICE](NOTICE).

If you want a mature product today, use Prospero Manager. If you want to build
a C++-native one, you are in the right place.

## What works today

- A CMake project that configures, builds and tests on a normal Linux machine,
  with no console and no SDK required. `-Wall -Wextra -Wpedantic -Wshadow
  -Wconversion -Wsign-conversion -Wold-style-cast -Werror` is on by default.
- A platform interface (`pandora::Platform`) with a working **host**
  implementation, so console logic can be written and unit-tested off-device.
- `pandora-core`: title ID validation, `contentVersion` parsing/ordering,
  `sce_sys/param.json` handling, configuration, logging, SHA-256 and a ZIP
  writer — all from the standard library.
- `pandora-http`: an HTTP server and API (`/api/health`, `/api/version`,
  `/api/fs/list`) with the UI compiled into the binary.
- `pandora-cli`: `title-id`, `param`, `digest`, `package`, `doctor`.
  `pandora-cli package pkg dist/PPSA77010.zip` produces the catalog-ready
  archive.
- 9 test suites, 190 assertions, all passing. Run them with `ctest`.

## What does not work yet

- Everything console-specific. [src/platform_console.cpp](src/platform_console.cpp)
  is a **placeholder that has never been run on hardware**, and the build
  refuses to pretend otherwise — see [docs/ROADMAP.md](docs/ROADMAP.md).
- Storage operations, PKG installation, payload launching, save data, thermal
  control, process management.
- The real frontend. The embedded page is a placeholder, not the finished UI.

## Build and run

Host build (no SDK needed):

```sh
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/pandora-http                  # serves http://127.0.0.1:8080
```

Or with `make`, which wraps the same commands:

```sh
make build && make test && make run
```

Package a title (this works today; it archives `pkg/`):

```sh
make package                          # -> dist/PPSA77010.zip
```

Console builds are **not wired up yet** — the toolchain file exists
([cmake/ps5-toolchain.cmake](cmake/ps5-toolchain.cmake)) and the platform
implementation is written, but nothing has been loaded on hardware. That is
milestone M0 in [docs/ROADMAP.md](docs/ROADMAP.md).
[docs/BUILDING.md](docs/BUILDING.md) explains what exists and what does not,
rather than pretending.

## Try the CLI

```sh
./build/pandora-cli title-id check PPSA77010
./build/pandora-cli param validate pkg/sce_sys/param.json
./build/pandora-cli digest dist/PPSA77010.zip
./build/pandora-cli doctor
```

## Repository layout

```
include/pandora/   Public headers. One per module; this is the contract.
src/               Implementation.
  version.cpp        contentVersion: parse, order, format.
  param.cpp          sce_sys/param.json and title ID rules.
  json.cpp           A small reader for the flat objects this project uses.
  config.cpp         Runtime configuration.
  sha256.cpp         SHA-256, verified against the published vectors.
  zip.cpp            The archive writer that produces a release asset.
  http.cpp           Routing, and the server itself.
  platform_host.cpp  The host platform. What the tests exercise.
  platform_console.cpp  The console platform. Compiled only for hardware. UNVALIDATED.
tools/pandora-cli/ Host-side tooling: validation, packaging, digest.
tests/             One suite per module, plus a packaging end-to-end test.
ui/index.html      The dashboard, compiled into the binary at build time.
pkg/sce_sys/       Native title metadata (param.json, icon0.png).
cmake/             Toolchain file and the packaging test.
docs/              Architecture, building, title IDs, releasing, roadmap.
```

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the design and the
reasoning behind the module split.

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

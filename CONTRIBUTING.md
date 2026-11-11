# Contributing to ps5-pandora

Thanks for being here. This project exists because people show up and improve
it, and there is useful work for every skill level — you do not have to write
C++ to make a real contribution.

This document is the contract between you and the project. It is deliberately
specific: follow it and your contribution moves quickly; skip it and a
maintainer has to ask you to redo work, which wastes your time.

- [Ways to contribute](#ways-to-contribute)
- [Before you start](#before-you-start)
- [Development environment](#development-environment)
- [Build, test and lint](#build-test-and-lint)
- [Project layout](#project-layout)
- [Coding standards](#coding-standards)
- [Commits and sign-off](#commits-and-sign-off)
- [Pull requests](#pull-requests)
- [What CI checks](#what-ci-checks)
- [Reporting bugs](#reporting-bugs)
- [Requesting features](#requesting-features)
- [Translations](#translations)
- [Hardware testing reports](#hardware-testing-reports)
- [Scope: what we will not accept](#scope-what-we-will-not-accept)
- [Licence of your contribution](#licence-of-your-contribution)
- [Getting help](#getting-help)

---

## Ways to contribute

You do not need to write code, and the non-code work is usually the most
neglected:

| What | How | Good for |
| --- | --- | --- |
| **Bug reports** | Open an issue with the bug template | Everyone |
| **Hardware testing** | Run a build on your console and report the result | Anyone with a jailbroken PS5 |
| **Documentation** | Fix or extend anything under `docs/` or in a README | Everyone |
| **Translations** | Add or improve a locale under `ui/` | Fluent speakers |
| **Design / UX** | Propose UI flows, icons, accessibility fixes | Designers |
| **Code** | Pick up an issue, especially `good first issue` | C++ developers |
| **Triage** | Reproduce reports, ask for missing detail, link duplicates | Regulars |
| **Code review** | Review open pull requests | Anyone who knows the area |

If you are unsure where to start, open a discussion or comment on an issue
asking for a pointer. That is a welcome question, not a nuisance.

## Before you start

**Small fixes** (typo, broken link, obvious bug) — open a pull request
directly. No issue needed.

**Anything substantial** — a new subsystem, a new dependency, a change to a
public header, a refactor across modules — **open an issue first** and wait for a
maintainer to agree on the approach. This is not gatekeeping: it is how we
avoid you spending a weekend on a design that will be rejected. [GOVERNANCE.md](GOVERNANCE.md)
explains how decisions are made.

**Never open a pull request you are not prepared to iterate on.** A
contribution that goes silent after review will be closed after 30 days.

## Development environment

### 1. A C++20 toolchain

You need a C++20 compiler and CMake 3.20 or newer. Tested with GCC 14.2.

```sh
# Debian / Ubuntu
sudo apt install build-essential cmake clang-format

# macOS
brew install cmake llvm clang-format
```

`clang-format` is worth installing: CI checks formatting, and `make fmt` fixes
it locally. `.clang-format` is committed and is the single source of truth.

### 2. Build for the host (no console required)

This is the normal loop for most work. The whole project builds and tests
without the PS5 Payload SDK:

```sh
git clone https://github.com/dantonious/ps5-pandora
cd ps5-pandora
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/pandora-http
```

Most logic — validation, parsing, routing, configuration, the platform *contract*
— is testable here. Write it that way: if a piece of code can be tested on the
host, it must be.

### 3. Build for hardware (optional, needs a console)

Only needed if you are working on the console platform layer or want to run a
build. Install the PS5 Payload SDK:

```sh
git clone --depth 1 https://github.com/ps5-payload-dev/sdk /opt/ps5-payload-sdk
cd /opt/ps5-payload-sdk
./build.sh          # downloads and builds the clang toolchain; takes a while
```

Then:

```sh
export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk
make console
cmake --build build-console -j
```

Full details, including common failures, are in [docs/BUILDING.md](docs/BUILDING.md).

### 4. Editor

Any editor works. `.editorconfig` and `.clang-format` are committed, so
formatting matches whatever you use. CMake exports
`build/compile_commands.json`, which is what an editor's C++ language server
reads — point it there and do not hand-write a second set of include paths.

## Build, test and lint

Everything runs through `make`, but the plain CMake equivalents are listed so you
can use whichever you prefer.

| Task | `make` | plain |
| --- | --- | --- |
| Configure | `make configure` | `cmake -S . -B build` |
| Format | `make fmt` | `clang-format -i $(git ls-files '*.cpp' '*.hpp')` |
| Lint | `make lint` | `cmake --build build -j` (warnings are errors) |
| Build | `make build` | `cmake --build build -j` |
| Test | `make test` | `ctest --test-dir build --output-on-failure` |
| Serve | `make run` | `./build/pandora-http` |
| Package title | `make package` | `./build/pandora-cli package pkg dist/PPSA77010.zip` |

**Before you push, run the same three gates CI runs:**

```sh
make fmt-check && make build && make test
```

A pull request that fails these will not be reviewed until they pass.

## Project layout

```
include/pandora/       Public headers. One per module; this is the contract.
  version.hpp            ContentVersion: parse, order, format.
  param.hpp              sce_sys/param.json + title ID rules.
  json.hpp               The small JSON reader this project needs, and no more.
  config.hpp             Runtime configuration.
  log.hpp                The log levels and the one logging entry point.
  sha256.hpp             SHA-256.
  zip.hpp                The release-archive writer.
  http.hpp               Request, Response, routing, the server.
  platform.hpp           The Platform interface.
src/
  version.cpp param.cpp json.cpp config.cpp log.cpp sha256.cpp zip.cpp
  http.cpp               Routing and the accept loop.
  main_http.cpp          The pandora-http entry point.
  platform_host.cpp      Development implementation (std::filesystem).
  platform_console.cpp   Console implementation. Compiled only for hardware. UNVALIDATED.
tools/pandora-cli/     Host-side tooling.
tests/                 One suite per module, plus the packaging end-to-end test.
ui/                    Frontend source, compiled into the binary.
pkg/sce_sys/           Native title metadata (param.json, icon0.png).
cmake/                 Toolchain file and the packaging test.
docs/                  Architecture, building, title IDs, releasing, roadmap.
```

The rule that keeps this maintainable: **`pandora-core` never depends on
`pandora-http-core`, `pandora-http` or `pandora-cli`, and console-specific code
never leaks outside `src/platform_console.cpp`.**

## Coding standards

These are enforced at review, and most of them are enforced by CI.

1. **`clang-format` is not negotiable.** CI runs `--dry-run --Werror`. `make fmt`
   fixes it; do not reflow code by hand to match a style you prefer.
2. **The build is `-Werror`, with `-Wconversion` and `-Wsign-conversion` on.**
   If a warning is genuinely wrong, justify a scoped `#pragma` with a comment
   saying why. Do not silence a conversion warning with a cast you have not
   checked — that is how a wrong number reaches the console.
3. **Every FFI declaration needs a comment** naming the symbol's origin and the
   contract you are relying on, and these belong in `src/platform_console.cpp`
   only. If you need one elsewhere, discuss it in an issue first.
4. **No exceptions on a path a user can reach.** A throwing payload on a console
   is a reboot. Use the `std::error_code` overloads of `<filesystem>`, not the
   throwing ones. `throw` is acceptable only in `main`-level argument parsing
   where the alternative is not starting at all.
5. **No new dependency without discussion.** This project links nothing but the
   standard library, and that is a feature. Open an issue with the rationale,
   the alternatives considered, and what it costs to cross-compile for the
   console — then expect to be asked to write it here instead.
6. **`pandora-core` must stay free of platform assumptions.** No sockets, no
   threads, no `<filesystem>` outside the platform implementations, nothing that
   assumes a hosted runtime. The console's libc++ is not the host's.
7. **Public declarations get doc comments.** A `///`-style comment on every type,
   function and constant in `include/pandora/`, saying what it does and what it
   assumes. No exceptions for "obvious" items — the next reader is not you.
8. **Tests live in `tests/`**, one suite per module, plus the end-to-end
   packaging test. A behaviour change without a test is an incomplete change. A
   bug fix starts with a test that fails.
9. **Errors are values with context.** Every fallible function takes a
   `std::string* error` and writes a sentence a user can act on. Do not return a
   bool with no way to find out what went wrong.
10. **Do not comment out code.** Delete it; git remembers.
11. **No new public API without a use.** A header is a contract; do not add a
    function "for later".

## Commits and sign-off

We use [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <short imperative summary>

<body: what changed and why, wrapped at 72 columns>

<footer: references, e.g. Closes #42>
```

Types: `feat`, `fix`, `docs`, `refactor`, `perf`, `test`, `build`, `ci`,
`chore`. Scope is the area: `core`, `http`, `cli`, `ui`, `pkg`, `docs`,
`platform`.

Examples:

```
feat(core): validate contentVersion against the NN.NNN.NNN rule
fix(http): return 404 instead of an empty 200 for an unknown path
docs(contributing): explain how to report hardware test results
```

**Every commit must be signed off** under the
[Developer Certificate of Origin](https://developercertificate.org/):

```sh
git commit -s -m "feat(core): validate contentVersion"
```

This appends:

```
Signed-off-by: Your Name <you@example.com>
```

The sign-off is your statement that you wrote the contribution, or have the
right to submit it, and that it may be distributed under this project's
licence. CI rejects commits without it. Set your identity first:

```sh
git config user.name  "Your Name"
git config user.email "you@example.com"
```

We use the DCO, **not a CLA**. You keep the copyright to your work; you grant
the project the licence described in [Licence of your contribution](#licence-of-your-contribution).

## Pull requests

1. **Fork** the repository and create a branch from `main`:

   ```sh
   git checkout -b feat/storage-listing
   ```

   Branch naming: `<type>/<short-slug>`, e.g. `fix/param-validation`,
   `docs/building-macos`.

2. **Keep it focused.** One logical change per pull request. Ten small pull
   requests get reviewed in a day; one 3,000-line pull request sits for weeks.

3. **Write the description** using the pull request template. Say what the
   change does, why, and how you tested it. Link the issue it closes
   (`Closes #42`).

4. **Tell us if it ran on hardware.** This is the single most valuable piece of
   information in a PS5 pull request. State the console model, firmware
   version, and how you loaded the payload — or say plainly that it has only
   been tested on the host.

5. **Push, then open the pull request against `main`.** CI starts immediately.

6. **Respond to review.** A maintainer will either approve, request changes, or
   explain why the change is out of scope. If you disagree, say so with
   reasoning — that is a normal technical discussion, not a conflict.

7. **Squash-merge is the default.** Your branch is merged as one commit; the
   pull request title becomes the commit subject, so make it a good
   Conventional Commit.

### Review expectations

- **We aim to give a first response within 5 working days.** This is a
  volunteer project; if a week passes with no comment, a polite ping on the
  pull request is appropriate and welcome.
- **Approval requires one maintainer.** Changes to `platform::ps5`, to the
  release pipeline, or to licensing require two.
- **A maintainer may push to your branch** to fix formatting or rebase. If you
  would rather they did not, say so in the pull request.

## What CI checks

| Job | What it does | Must pass to merge |
| --- | --- | --- |
| `format` | `clang-format --dry-run --Werror` over every source | Yes |
| `build` | Configure + build on Linux and macOS, warnings as errors | Yes |
| `test` | `ctest` on Linux and macOS | Yes |
| `gcc` / `clang` | Builds with both compilers | Yes |
| `sanitizers` | Builds and tests with ASan and UBSan | Yes |
| `signoff` | Every commit has a `Signed-off-by` trailer | Yes |
| `package` | Runs `pandora-cli package` and checks the archive layout with an independent zip reader | Yes |

Console builds are **not** in CI: they need the SDK and a licensed toolchain.
That is exactly why step 4 above matters.

## Reporting bugs

Open an issue with the **Bug report** template. A useful report contains:

- What you did, what you expected, what happened.
- The exact build: `./build/pandora-cli version`, and whether it ran on the host
  or on a console (model + firmware version).
- Logs. Raise the log level first: `./build/pandora-http --log debug`.
- A minimal reproduction, if you can find one.

Please do **not** paste copyrighted console dumps, keys, or personal data. If a
log contains a serial number or account ID, redact it.

## Requesting features

Open an issue with the **Feature request** template and describe the *problem*,
not only the solution: what are you trying to accomplish, what blocks you
today, and what have you tried. A well-framed problem often has a better
solution than the one originally imagined.

Features are prioritised in [docs/ROADMAP.md](docs/ROADMAP.md). Commenting on an
existing issue is more useful than opening a duplicate.

## Translations

UI strings are collected under `ui/locales/`, one flat `key: value` file per
language. The page still renders English inline and the lookup is not wired yet,
so a translation today is the input for that work rather than a change users see
immediately. To add a language:

1. Copy `en.json` to `<locale>.json` and translate the values. Do not translate
   the keys.
2. Right-to-left languages (Arabic, Persian, Hebrew) also need the `rtl: true`
   flag.
3. Open a pull request; a native speaker will review it before merge.

Partial translations are accepted — an untranslated key falls back to English.

## Hardware testing reports

If you have a jailbroken PS5, you are more valuable to this project than you
probably realise. A hardware report is a first-class contribution.

Open an issue titled `hardware: <build> on <firmware>` containing:

- Console model and **exact firmware version**.
- The payload loader you used and how you loaded the ELF.
- The build you tested: commit hash or release tag.
- What worked, what did not, and the log output.
- Whether the console needed a reboot afterwards.

Negative results are as useful as positive ones. "It crashes on load with
firmware X" is a genuine contribution.

## Scope: what we will not accept

To keep the project legally clean and welcome to contributors:

- **Code copied from another project.** This codebase is written from scratch.
  Do not paste GPL, MIT or unlicensed code from elsewhere, including from
  Prospero Manager. If you want to port an *idea*, describe the behaviour in an
  issue and implement it independently. If you must reference another
  implementation, say so in the issue **before** writing code, so the licensing
  can be assessed.
- **Reimplementations of proprietary Sony code or leaked SDK material.** Use
  the public PS5 Payload SDK only.
- **Code that cannot be built with `-Werror`.** The build is the first review.
- **Piracy features.** Anything whose purpose is obtaining or installing
  content the user does not own. PKG installation of the user's own packages is
  in scope; circumventing entitlement is not.
- **Telemetry or phone-home behaviour.** The dashboard runs on a local network
  and talks to the console. It does not report to us.
- **Dependencies with licences incompatible with GPL-3.0.**

## Licence of your contribution

This project is licensed under the **GNU General Public License, version 3 or
later** ([LICENSE](LICENSE)).

By submitting a contribution you agree that:

1. You have the right to submit it.
2. It is licensed to the project and to everyone else under
   **GPL-3.0-or-later** (inbound = outbound).
3. You keep your copyright. You are not assigning it.
4. Your commit carries a `Signed-off-by` trailer confirming the above
   ([DCO](https://developercertificate.org/)).

If you contribute on behalf of an employer, make sure you are permitted to.

## Getting help

- **Questions about using or building the project** — open a discussion.
- **Questions about contributing to a specific area** — comment on the relevant
  issue.
- **Anything sensitive** — see [SECURITY.md](SECURITY.md), never a public issue.

Nobody here is paid to answer, so be patient and be kind. A well-researched
question ("I read docs/BUILDING.md and tried X, here is the error") gets a
much faster answer than "it doesn't work".

## Recognition

Contributors are credited in the release notes for the version their work
lands in. Sustained contributors are invited to become reviewers, and reviewers
to become maintainers — see [GOVERNANCE.md](GOVERNANCE.md).

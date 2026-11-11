# Contributing to ps5-pandora

Thanks for being here. This project exists because people show up and improve
it, and there is useful work for every skill level — you do not have to write
Rust to make a real contribution.

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
| **Translations** | Add or improve a locale under `ui/src/i18n/` | Fluent speakers |
| **Design / UX** | Propose UI flows, icons, accessibility fixes | Designers |
| **Code** | Pick up an issue, especially `good first issue` | Rust developers |
| **Triage** | Reproduce reports, ask for missing detail, link duplicates | Regulars |
| **Code review** | Review open pull requests | Anyone who knows the area |

If you are unsure where to start, open a discussion or comment on an issue
asking for a pointer. That is a welcome question, not a nuisance.

## Before you start

**Small fixes** (typo, broken link, obvious bug) — open a pull request
directly. No issue needed.

**Anything substantial** — a new subsystem, a new dependency, a change to the
public API, a refactor across crates — **open an issue first** and wait for a
maintainer to agree on the approach. This is not gatekeeping: it is how we
avoid you spending a weekend on a design that will be rejected. [GOVERNANCE.md](GOVERNANCE.md)
explains how decisions are made.

**Never open a pull request you are not prepared to iterate on.** A
contribution that goes silent after review will be closed after 30 days.

## Development environment

### 1. Rust

Install a stable toolchain with [rustup](https://rustup.rs/):

```sh
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
rustup component add rustfmt clippy
```

The workspace pins the toolchain in `rust-toolchain.toml`; `rustup` will honour
it automatically. Minimum supported Rust version is declared in `Cargo.toml`
(`rust-version`) and enforced in CI.

### 2. Build for the host (no console required)

This is the normal loop for most work. The whole workspace builds and tests
without the PS5 Payload SDK:

```sh
git clone https://github.com/dantonious/ps5-pandora
cd ps5-pandora
cargo test --workspace
cargo run -p pandora-http
```

Most logic — validation, parsing, routing, configuration, the platform *contract*
— is testable here. Write it that way: if a piece of code can be tested on the
host, it must be.

### 3. Build for hardware (optional, needs a console)

Only needed if you are working on the `ps5` platform layer or want to run a
build. Install the PS5 Payload SDK:

```sh
git clone --depth 1 https://github.com/ps5-payload-dev/sdk /opt/ps5-payload-sdk
cd /opt/ps5-payload-sdk
./build.sh          # downloads and builds the clang toolchain; takes a while
```

Then:

```sh
export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk
make build-hw
```

Full details, including common failures, are in [docs/BUILDING.md](docs/BUILDING.md).

### 4. Editor

Any editor works. `.editorconfig`, `rustfmt.toml` and `clippy.toml` are
committed, so formatting matches whatever you use. For VS Code,
`rust-analyzer` plus `cargo clippy` on save is what most of us run.

## Build, test and lint

Everything runs through `make`, but the plain `cargo` equivalents are listed so
you can use whichever you prefer.

| Task | `make` | `cargo` |
| --- | --- | --- |
| Format | `make fmt` | `cargo fmt --all` |
| Lint | `make lint` | `cargo clippy --workspace --all-targets -- -D warnings` |
| Test | `make test` | `cargo test --workspace` |
| Build | `make build` | `cargo build --workspace` |
| Docs | `make doc` | `cargo doc --workspace --no-deps --open` |
| Package title | `make package` | `cargo run -p pandora-cli -- package` |

**Before you push, run the same three gates CI runs:**

```sh
make fmt && make lint && make test
```

A pull request that fails these will not be reviewed until they pass.

## Project layout

```
crates/pandora-core/   Domain types and the platform abstraction.
  src/version.rs         ContentVersion: parse, order, format.
  src/param.rs           sce_sys/param.json + title ID rules.
  src/config.rs          Runtime configuration.
  src/platform/mod.rs    The Platform trait.
  src/platform/host.rs   Development implementation (std::fs).
  src/platform/ps5.rs    Console implementation. cfg-gated; UNVALIDATED.
crates/pandora-http/   Axum router, API handlers, embedded UI.
crates/pandora-cli/    Host-side tooling.
ui/                    Frontend sources.
pkg/sce_sys/           Native title metadata (param.json, icon0.png).
docs/                  Architecture, building, title IDs, releasing, roadmap.
```

The rule that keeps this maintainable: **`pandora-core` never depends on
`pandora-http` or `pandora-cli`, and console-specific code never leaks outside
`platform::ps5`.**

## Coding standards

These are enforced at review, and most of them are enforced by CI.

1. **`cargo fmt` is not negotiable.** CI runs `--check`.
2. **`clippy -D warnings` is not negotiable.** If a lint is genuinely wrong,
   justify an `#[allow(...)]` on the smallest possible scope, with a comment
   saying why.
3. **Every `unsafe` block needs a `// SAFETY:` comment** explaining why the
   block is sound: pointer validity, lifetime, aliasing, alignment. An
   unexplained `unsafe` block is rejected on sight. Keep `unsafe` inside
   `platform::ps5`; if you need it elsewhere, discuss it in an issue first.
4. **No `unwrap()` / `expect()` / `panic!()` on a path a user can reach.** In
   tests and in provably-total cases (e.g. a literal parses), it is fine. Use
   `Result` and the `PandoraError` type.
5. **No new dependency without discussion.** Rust dependencies are cheap to add
   and expensive to remove, and every one must be GPL-3.0-compatible. Open an
   issue with the rationale, alternatives considered, and why the stdlib or an
   existing dependency will not do.
6. **`pandora-core` must stay `no_std`-friendly.** Do not pull in `tokio`,
   `std::net` or anything else that assumes a hosted runtime. `std` may be
   used only where it is behind a feature or in the host platform module.
7. **Public items get doc comments.** `///` on every public type, function and
   module, saying what it does and what it assumes. No exceptions for
   "obvious" items — the next reader is not you.
8. **Tests live next to the code** in `#[cfg(test)] mod tests`, plus integration
   tests under `tests/`. A behaviour change without a test is an incomplete
   change.
9. **Errors are typed, not stringly.** Return `PandoraError` variants; do not
   format an error message and return it as a `String`.
10. **Do not comment out code.** Delete it; git remembers.

## Commits and sign-off

We use [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <short imperative summary>

<body: what changed and why, wrapped at 72 columns>

<footer: references, e.g. Closes #42>
```

Types: `feat`, `fix`, `docs`, `refactor`, `perf`, `test`, `build`, `ci`,
`chore`. Scope is the area: `core`, `http`, `cli`, `ui`, `pkg`, `docs`.

Examples:

```
feat(core): validate contentVersion against the NN.NNN.NNN rule
fix(http): return 404 instead of panicking on an unknown path
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
| `fmt` | `cargo fmt --all --check` | Yes |
| `clippy` | `cargo clippy --workspace --all-targets -- -D warnings` | Yes |
| `test` | `cargo test --workspace` on Linux, macOS and Windows | Yes |
| `msrv` | Builds with the declared minimum Rust version | Yes |
| `signoff` | Every commit has a `Signed-off-by` trailer | Yes |
| `package` | Runs `pandora package` and checks the archive layout | Yes |

Console builds are **not** in CI: they need the SDK and a licensed toolchain.
That is exactly why step 4 above matters.

## Reporting bugs

Open an issue with the **Bug report** template. A useful report contains:

- What you did, what you expected, what happened.
- The exact build: `pandora --version`, and whether it ran on the host or on a
  console (model + firmware version).
- Logs. Raise the log level first: `RUST_LOG=debug`.
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

The UI is translated through locale files under `ui/src/i18n/`. To add a
language:

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

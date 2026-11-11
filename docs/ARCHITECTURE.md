# Architecture

## The shape of the thing

One program runs on the console. It serves an HTTP dashboard and does the work
locally. There is no server-side component anywhere else, no account, and no
network dependency beyond the console's own LAN.

```
        phone / tablet / desktop      (or the console's own browser)
                  |
                  | HTTP (LAN)
                  v
     +-------------------------------+
     |  pandora-http                 |   routing + the server
     |    /api/*   /                 |   the dashboard is compiled in
     +---------------+---------------+
                     |
                     v
     +-------------------------------+
     |  pandora-core                 |   config, metadata, version rules,
     |    pandora::Platform          |   JSON, SHA-256, ZIP, logging
     +---------------+---------------+
                     |
          +----------+----------+
          v                     v
   platform_host.cpp     platform_console.cpp
   (development)         (the console, UNVALIDATED)
```

## Why these four targets

The split is not architectural decoration. Each boundary buys something concrete:

**`pandora-core`** holds everything that can be reasoned about without a console:
metadata rules, version ordering, configuration, hashing, archive writing, and
the `Platform` interface. It links nothing but the standard library, so its tests
run in milliseconds and its rules cannot be broken by a change in the server.

**`pandora-http-core`** is the routing and the server. Routing does not know
about sockets — `route_request` takes a `Request` and a `Platform&` and returns a
`Response` — which is what lets the routing tests cover every status code without
opening a port.

**`pandora-http`** is the executable: argument parsing, log setup, and the accept
loop.

**`pandora-cli`** is host-side tooling — the release pipeline. It must work on a
machine with no console and no SDK, because that is the machine a contributor
has. `pandora-cli package` is what produces the catalog artifact.

The dependency rule, enforced by review and by CMake:

```
pandora-cli       ──> pandora-core
pandora-http-core ──> pandora-core
pandora-http      ──> pandora-http-core
pandora-core      ──> (nothing in this project)
```

## Why a Platform interface

Console code cannot be unit-tested on a laptop, and code that cannot be tested
rots. Putting every OS operation behind one interface means:

- `platform_host.cpp` implements it with `std::filesystem` and runs in the tests
  and in `./build/pandora-http` on a development machine.
- `platform_console.cpp` implements it against `libkernel` and is the only file
  in the project that declares FFI symbols.
- A bug in the console layer surfaces as a failing test in one module, not as
  corruption three layers up.

The interface is deliberately small — `name`, `data_root`, `list_dir`,
`read_file`, `write_file`, `ensure_dir`. Every method added to it is a method
that must be implemented twice and kept in sync, so it grows only when a feature
needs it.

## Why the UI is compiled in

`CMakeLists.txt` reads `ui/index.html` at configure time and writes it into a
header as a string literal. The document is part of the executable: one file to
deploy, no static-asset path to get wrong, and no way for the UI and the API to
be different versions.

The alternative — reading the file at start-up — would add a directory the
program cannot run without. The console may have no such directory to read from.

## Why there are no dependencies

The JSON reader, the SHA-256 implementation, the CRC-32 and ZIP writer, and the
HTTP server are all in this repository. That is a deliberate cost: roughly a
thousand lines of code that a library could have provided.

What it buys: a build that cannot break because an upstream package changed, an
auditable supply chain, and nothing to cross-compile before the console build can
even start. On a platform where the toolchain is the hard part, every removed
dependency is a removed variable. See [NOTICE](../NOTICE).

## Versioning: two numbers, different jobs

| | Release tag | `contentVersion` |
| --- | --- | --- |
| Looks like | `v0.2.0`, anything | always `01.000.070` |
| Decides | what humans read | whether a console offers an update |
| Compared how | not compared | as three numbers, left to right |

The console compares `contentVersion` against the installed copy. A release that
does not raise it is invisible as an update, no matter what the tag says. This is
why `pandora-cli param bump` exists and why [RELEASING.md](RELEASING.md) makes it
a checklist item rather than a convention.

Field widths are exact: two digits, three digits, three digits. `1.0.0` is
rejected, not padded, because a malformed value is indistinguishable from an
unknown one. A bump clamps at the field width (`01.000.999`, not `01.000.1000`),
which is a bug this project already had once — see `tests/version_test.cpp`.

## Failure philosophy

- **Return values, not exceptions, on reachable paths.** A throwing payload on a
  console is a console that needs a reboot. `std::filesystem` errors are taken
  through the `std::error_code` overloads, never the throwing ones.
- **Optional, not sentinel.** "Absent" and "present but empty" are different
  things for an HTTP query parameter and for a JSON field, so they are different
  types (`std::optional<std::string>`).
- **Errors are values with context.** Every fallible function takes a
  `std::string* error` and writes a sentence a user can act on.
- **Warnings are errors.** The build is `-Werror` with `-Wconversion` and
  `-Wsign-conversion` on, so a narrowing mistake fails the build rather than
  producing a wrong number on the console.
- **FFI is confined and justified.** Console calls live only in
  `platform_console.cpp`.

## What is deliberately not here yet

An authentication layer. The dashboard binds to `127.0.0.1` by default and
trusts anything that reaches it, which is the same trust model the console
ecosystem uses — but it is a decision worth stating explicitly rather than
discovering. Binding to `0.0.0.0` is an opt-in (`--bind` or `pandora.json`), not
the default. Do not expose this to an untrusted network.

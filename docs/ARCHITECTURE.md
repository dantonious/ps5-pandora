# Architecture

## The shape of the thing

One payload runs on the console. It serves an HTTP dashboard and does the work
locally. There is no server-side component anywhere else, no account, and no
network dependency beyond the console's own LAN.

```
        phone / tablet / desktop
                  |
                  | HTTP (LAN)
                  v
     +---------------------------+
     |  pandora-http             |   axum router + embedded UI
     |    /api/*   /             |
     +------------+--------------+
                  |
                  v
     +---------------------------+
     |  pandora-core             |   config, metadata, version rules
     |    platform::Platform     |   <- the only place that touches the OS
     +------------+--------------+
                  |
        +---------+---------+
        v                   v
   platform::host      platform::ps5
   (development)       (the console, UNVALIDATED)
```

## Why three crates

The split is not architectural decoration. Each boundary buys something concrete:

**`pandora-core`** holds everything that can be reasoned about without a console:
metadata rules, version ordering, configuration, and the `Platform` trait. It has
no async runtime and no HTTP dependency, which means its tests run in
milliseconds and its rules cannot be broken by a change in the server.

**`pandora-http`** is the transport. It depends on `pandora-core`, never the
reverse. Handlers stay thin: parse, call the platform, render. Business rules
belong in core, where they are testable without a socket.

**`pandora-cli`** is host-side tooling — the release pipeline. It must work on a
machine with no console and no SDK, because that is the machine a contributor
has. `pandora package` is what produces the catalog artifact.

The dependency rule, enforced by review:

```
pandora-cli ──> pandora-core
pandora-http ──> pandora-core
pandora-core ──> (nothing in this workspace)
```

## Why the Platform trait

Console code cannot be unit-tested on a laptop, and code that cannot be tested
rots. Putting every OS operation behind one trait means:

- `platform::host` implements it with `std::fs` and runs in tests and in
  `cargo run` on a development machine.
- `platform::ps5` implements it against `libkernel` and is the only module that
  contains `unsafe`.
- A bug in the console layer surfaces as a failing test in one module, not as
  corruption three layers up.

The trait is deliberately small. Every method added to it is a method that must
be implemented twice and kept in sync, so it grows only when a feature needs it.

## Why the UI is embedded

`include_str!` puts the page in the binary. One file to deploy, no static-asset
path to get wrong, no way for the UI and the API to be different versions.

That is fine for the placeholder page. When the real frontend arrives it will be
a Vite build embedded the same way, and the generated bundle will be committed
or produced by the build — not fetched at runtime, because the console may have
no internet access at all.

## Why `no_std`-friendly core

The console is a FreeBSD-derived target with a partial libc surface. A crate
that assumes a full hosted runtime is a crate that may not run there. Keeping
`pandora-core` free of `std`-only facilities (other than in `platform::host`)
means the part that encodes the rules is portable by construction.

## Versioning: two numbers, different jobs

| | Release tag | `contentVersion` |
| --- | --- | --- |
| Looks like | `v0.2.0`, anything | always `01.000.070` |
| Decides | what humans read | whether a console offers an update |
| Compared how | not compared | as three numbers, left to right |

The console compares `contentVersion` against the installed copy. A release that
does not raise it is invisible as an update, no matter what the tag says. This is
why `pandora param bump` exists and why [RELEASING.md](RELEASING.md) makes it a
checklist item rather than a convention.

## Failure philosophy

- **Typed errors**, never strings, so callers can distinguish "not found" from
  "invalid" and return the right status code.
- **No panics on reachable paths.** A panic in a payload on a console is a
  console that needs a reboot.
- **`unsafe` is confined and justified.** Every block carries a `// SAFETY:`
  comment stating the invariant. A wrong comment is a bug of the same severity
  as wrong code.

## What is deliberately not here yet

An auth layer. The dashboard currently trusts anything on the LAN, which is the
same trust model the console ecosystem uses and is still a decision worth making
explicitly: see `docs/ROADMAP.md`. Do not expose this to an untrusted network.

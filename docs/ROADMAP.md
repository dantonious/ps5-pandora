# Roadmap

Ordered by risk, not by appeal. The things most likely to kill the project come
first.

## M0 — Prove Rust can run on the console

**The question:** can a `no_std` Rust binary load and execute as a PS5 payload?

This is the gate. Everything else is wasted effort if the answer is no.

- Custom target spec for the console ABI.
- Build a payload that prints one line to stdout and exits.
- Load it on hardware; record the result in a hardware issue.

**Done when:** a Rust binary runs on a real console and its output is captured.

## M1 — Reach the kernel, not just the CPU

Prove the FFI layer works for the operations the product needs.

- `sceKernelOpen/Close/Read/Write/Getdents` through `platform::ps5`.
- List a directory and read a file on hardware.
- Confirm the directory framing assumption in `ps5.rs` against real output.

**Done when:** a hardware report shows a correct directory listing.

## M2 — Serve a dashboard from the console

- TCP listener on the console.
- `axum` (or a hand-rolled HTTP layer if allocation constraints demand it) on
  the target.
- `/api/health` reachable from a browser on the LAN.

**Done when:** a browser on the LAN loads the page served by the payload.

## M3 — Be installable as a native title

- Produce the title folder (`eboot.bin`, `sce_sys/param.json`, `sce_sys/icon0.png`).
- Reserve a title ID and replace the `PPSA77010` placeholder.
- `pandora package` output installs and launches from the home screen.

**Done when:** the app launches as its own title, not as a payload sent by hand.

## M4 — Storage

- Browse, upload, download, rename, copy, move, delete.
- Long operations as background jobs with progress.
- Path containment: nothing escapes the configured root.

## M5 — Payload management

- Upload, list, launch, stop payloads.
- Autoload sequence on boot.

## M6 — Packages, saves, telemetry

- PKG installation from local storage and from a stream.
- Save data backup and restore.
- Temperature and fan reporting where the environment exposes it.

## Non-goals

- Anything that circumvents entitlement.
- Telemetry that leaves the LAN.
- A hosted service.

## How to pick up work

Each milestone becomes issues. Issues labelled `good first issue` are scoped to
be finishable in an evening by someone who has read
[CONTRIBUTING.md](../CONTRIBUTING.md). If a milestone item has no issue yet and
you want it, open one — that is how it gets scoped.

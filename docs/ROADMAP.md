# Roadmap

Ordered by risk, not by appeal. The things most likely to kill the project come
first.

## M0 — Prove the console build runs

**The question:** can a binary produced from this repository load and execute on
a real PS5?

This is the gate. Everything else is wasted effort if the answer is no. The
toolchain file and `src/platform_console.cpp` exist; nothing has been run.

- Build with `cmake/ps5-toolchain.cmake` and load the payload.
- Capture the output, or the absence of it, in a hardware issue.
- Confirm which libkernel symbols the `extern "C"` block needs, and whether the
  names are right.

**Done when:** a hardware report shows the program running on a console, with its
output captured. A report that says "it crashes" is a valid and useful result.

## M1 — Reach the kernel, not just the CPU

Prove the FFI layer works for the operations the product needs.

- `sceKernelOpen/Close/Read/Write/Getdents` through `platform_console.cpp`.
- List a directory and read a file on hardware.
- Check the directory-entry framing assumption (records padded to four bytes)
  against real output. It is an assumption, not a measurement.

**Done when:** a hardware report shows a correct directory listing.

## M2 — Serve the dashboard from the console

- Bind a TCP listener on the console.
- Reach `/api/health` from the console's own browser.
- Confirm `std::filesystem` works on the console target, or replace it in
  `platform_console.cpp` with direct libkernel calls. **This is the main open
  assumption in the current code**: the console implementation uses
  `std::filesystem` for path handling and directory creation, and the console's
  libc++ may not provide it.

**Done when:** a browser on the LAN loads the page served by the payload.

## M3 — Be installable as a native title

- Produce the title folder (`eboot.bin`, `sce_sys/param.json`,
  `sce_sys/icon0.png`).
- Reserve a title ID and replace the `PPSA77010` placeholder.
- `pandora-cli package` output installs and launches from the home screen.

**Done when:** the app launches as its own title, not as a payload sent by hand.

## M4 — Storage

- Browse, upload, download, rename, copy, move, delete.
- Long operations as background jobs with progress.
- Path containment: nothing escapes the configured root. The current
  `/api/fs/list` takes whatever path it is given, which is fine on loopback and
  not fine on a LAN.

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

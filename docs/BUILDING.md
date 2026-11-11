# Building

## Host build (what most contributors need)

No console, no SDK, no special toolchain.

```sh
rustup component add rustfmt clippy
cargo build --workspace
cargo test  --workspace
cargo run -p pandora-http
```

The server listens on `0.0.0.0:7070` and uses the host platform, rooted at
`./data`. Open <http://127.0.0.1:7070>.

## Hardware build

> **Status: not wired up yet.** The console target is the first milestone in
> [ROADMAP.md](ROADMAP.md). Today you can build the crates for the host and
> package a title folder; producing a console `eboot.bin` from this workspace is
> not implemented. This section documents the intended path so the work has a
> target, and says plainly what does not exist.

### Intended toolchain

Rust needs a target definition for the console's ABI. That means:

1. The PS5 Payload SDK's clang toolchain, which knows the target triple and
   ships the `libkernel` stubs.
2. A custom target spec (`.json`) telling rustc to use that linker and to build
   a static, position-independent ELF without a hosted runtime.
3. `-Z build-std` to compile `core` for that target, since no prebuilt std
   exists.

Relevant work is tracked there, not here. If you want to help, that issue is the
highest-value thing in the project.

### Installing the SDK anyway (needed for FFI work)

```sh
git clone --depth 1 https://github.com/ps5-payload-dev/sdk /opt/ps5-payload-sdk
cd /opt/ps5-payload-sdk
./build.sh
export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk
```

`./build.sh` downloads and builds LLVM for the target. Expect a long first run
and several GB of disk.

Verify:

```sh
ls "$PS5_PAYLOAD_SDK/bin" | head
```

You should see `prospero-clang` and `prospero-clang++`.

### Common failures

| Symptom | Cause |
| --- | --- |
| `PS5_PAYLOAD_SDK: unbound variable` in a Makefile | The SDK is not exported in the current shell. |
| `prospero-clang: not found` | The SDK build did not finish; re-run `./build.sh`. |
| Linker errors about `libc` | You are compiling for the host but linking against the SDK, or the reverse. |
| Out of disk during `./build.sh` | The LLVM build needs several GB. |

## Packaging a title

```sh
cargo run -p pandora-cli -- param validate pkg/sce_sys/param.json
cargo run -p pandora-cli -- package --source pkg --out dist
```

Output: `dist/<TITLEID>.zip` with `<TITLEID>/` at the top of the archive, which
is the layout the console and the homebrew catalog both expect. The CLI prints
the SHA-256 you need for a catalog record.

## Checking your environment

```sh
cargo run -p pandora-cli -- doctor
```

Reports whether the SDK is visible, whether `pkg/sce_sys/param.json` validates,
and whether the icon is present.

## Reproducible builds

Release builds use `opt-level = "z"`, LTO, `codegen-units = 1`, `panic = "abort"`
and `strip = true` (see the workspace `Cargo.toml`), because payload size matters
and a stripped ELF is easier to load. Do not change those settings in a feature
pull request; they are a release decision.

# Building

## Host build (what most contributors need)

No console, no SDK, no special toolchain. A C++20 compiler and CMake 3.20 or
newer.

```sh
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/pandora-http
```

The equivalent through `make`:

```sh
make build        # configure + build
make test         # build + ctest
make run          # build + serve on http://127.0.0.1:8080
```

The server listens on `127.0.0.1:8080` and uses the host platform, rooted at
`./data` (created on first start). Pass `--bind 0.0.0.0` to reach it from
another device on your LAN, and read the note at the end of
[ARCHITECTURE.md](ARCHITECTURE.md) first.

### Compiler requirements

| | |
| --- | --- |
| Standard | C++20 (`std::optional`, `<=>`, designated initialisers) |
| Tested with | GCC 14.2 |
| CMake | 3.20+ |

The build enables `-Wall -Wextra -Wpedantic -Wshadow -Wconversion
-Wsign-conversion -Wnon-virtual-dtor -Wold-style-cast -Werror` on GCC and Clang.
Warnings are errors by default; pass `-DPANDORA_WARNINGS_AS_ERRORS=OFF` only to
diagnose a toolchain you are porting.

```sh
cmake -S . -B build -DPANDORA_WARNINGS_AS_ERRORS=OFF
```

### Formatting

```sh
make fmt          # rewrite in place
make fmt-check    # what CI runs; fails on any difference
```

`.clang-format` is committed and is the single source of truth. Formatting is
not a matter of taste here — a formatted tree removes a whole category of review
comments.

## Hardware build

> **Status: not wired up yet.** The toolchain file
> ([cmake/ps5-toolchain.cmake](../cmake/ps5-toolchain.cmake)) and the platform
> implementation ([src/platform_console.cpp](../src/platform_console.cpp)) both
> exist, but **nothing has been loaded on hardware**. This section documents the
> intended path so the work has a target, and says plainly what has not been
> proven. Do not install it on a console you care about.

### Intended toolchain

The PS5 Payload SDK's `prospero-clang++` is a wrapper that already knows the
target ABI, the sysroot and the import stubs, so CMake does not have to guess:

```sh
./scripts/bootstrap-sdk.sh
export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk

cmake -S . -B build-console \
  -DCMAKE_TOOLCHAIN_FILE=cmake/ps5-toolchain.cmake \
  -DPANDORA_BUILD_CONSOLE=ON \
  -DPANDORA_BUILD_TESTS=OFF
cmake --build build-console -j
```

Or `make console`.

The output is a static, position-independent ELF with no hosted runtime. Loading
it is a separate problem: it depends on your firmware and your loader, which is
why the first milestone in [ROADMAP.md](ROADMAP.md) is a hardware test and not
more code.

### Installing the SDK

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
| `PS5_PAYLOAD_SDK is not set` | The variable is not exported in the current shell. |
| `prospero-clang++ not found` | The SDK build did not finish; re-run `./build.sh`. |
| `CMakeSystem.cmake` errors after switching between host and console builds | Reuse one build directory per target, or delete the directory and reconfigure. |
| Linker errors about `libc` | You are compiling for the host but linking against the SDK, or the reverse. |
| Out of disk during `./build.sh` | The LLVM build needs several GB. |

## Packaging a title

```sh
./build/pandora-cli param validate pkg/sce_sys/param.json
./build/pandora-cli package pkg dist/PPSA77010.zip
```

Or `make package`.

Output: `dist/<TITLEID>.zip` with `<TITLEID>/` at the top of the archive, which
is the layout the console and the homebrew catalog both expect. The CLI prints
the SHA-256 you need for a catalog record. The writer is deterministic — the same
input produces the same bytes on any machine — because a catalog record pins that
hash.

The archive is stored, not compressed. It is dominated by the payload binary, and
storing keeps the writer free of a compression dependency it would otherwise have
to cross-compile. `python3 -c "import zipfile; zipfile.ZipFile('dist/PPSA77010.zip').testzip()"`
verifies it with an independent implementation.

## Checking your environment

```sh
./build/pandora-cli doctor
```

Runs self-checks with real expected values: the SHA-256 and CRC-32 published test
vectors, the `contentVersion` field-width clamp, the title ID rules, and the
platform in use. It reports the data root as information rather than a failure,
because the server creates it on start-up.

## Reproducible builds

Release configuration is `-O3 -DNDEBUG` with `CMAKE_BUILD_TYPE=Release`
(the default). There is no LTO or PGO yet: those are a release decision and
should be measured on the console before they are enabled, not assumed to help.
Do not change optimisation settings in a feature pull request.

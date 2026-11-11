#!/usr/bin/env bash
# Installs the PS5 Payload SDK into ${PS5_PAYLOAD_SDK:-/opt/ps5-payload-sdk}.
#
# Only needed to work on the console platform layer, or to build a native title.
# Host development does not require it.
set -euo pipefail

SDK_DIR="${PS5_PAYLOAD_SDK:-/opt/ps5-payload-sdk}"

if [[ -x "${SDK_DIR}/bin/prospero-clang" ]]; then
  echo "SDK already present at ${SDK_DIR}"
else
  if [[ -e "${SDK_DIR}" ]]; then
    echo "error: ${SDK_DIR} exists but does not look like the SDK." >&2
    echo "       Remove it or set PS5_PAYLOAD_SDK to another path." >&2
    exit 1
  fi

  echo "Cloning the PS5 Payload SDK into ${SDK_DIR}"
  git clone --depth 1 https://github.com/ps5-payload-dev/sdk "${SDK_DIR}"
  echo "Building the toolchain (this takes a while and several GB)"
  ( cd "${SDK_DIR}" && ./build.sh )
fi

cat <<EOF

Done. Add this to your shell profile:

  export PS5_PAYLOAD_SDK="${SDK_DIR}"

Then check the environment:

  make doctor
EOF

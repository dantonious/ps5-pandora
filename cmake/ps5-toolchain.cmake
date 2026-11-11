# CMake toolchain file for the PS5 console build.
#
#     export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk
#     cmake -S . -B build-console \
#       -DCMAKE_TOOLCHAIN_FILE=cmake/ps5-toolchain.cmake \
#       -DPANDORA_BUILD_CONSOLE=ON \
#       -DPANDORA_BUILD_TESTS=OFF
#
# UNVALIDATED. No one has run the result on hardware yet. This file exists so the
# console build is one command instead of an afternoon of guessing, and so the
# assumptions it makes are written down where they can be checked.

if(NOT DEFINED ENV{PS5_PAYLOAD_SDK})
  message(FATAL_ERROR
    "PS5_PAYLOAD_SDK is not set. Install the SDK first:\n"
    "  ./scripts/bootstrap-sdk.sh\n"
    "  export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk")
endif()

set(PANDORA_SDK "$ENV{PS5_PAYLOAD_SDK}")

if(NOT EXISTS "${PANDORA_SDK}/bin/prospero-clang++")
  message(FATAL_ERROR
    "${PANDORA_SDK}/bin/prospero-clang++ not found. The SDK build probably did "
    "not finish; re-run ${PANDORA_SDK}/build.sh")
endif()

set(CMAKE_SYSTEM_NAME FreeBSD)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   "${PANDORA_SDK}/bin/prospero-clang")
set(CMAKE_CXX_COMPILER "${PANDORA_SDK}/bin/prospero-clang++")

# The SDK's compiler wrapper already knows the sysroot and the linker flags; it
# only has to not be second-guessed here.
set(CMAKE_CXX_FLAGS_INIT "")

# Static, position-independent, no host runtime.
add_compile_options(-fPIC -ffreestanding)
add_link_options(-static)

set(CMAKE_FIND_ROOT_PATH "${PANDORA_SDK}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# End-to-end check: the CLI must produce an archive rooted at the title folder,
# and its top-level name must match the title id in param.json.
#
# Invoked by CTest with -DPANDORA_CLI= -DPANDORA_PKG= -DPANDORA_OUT=.

if(NOT PANDORA_CLI OR NOT PANDORA_PKG OR NOT PANDORA_OUT)
  message(FATAL_ERROR "PANDORA_CLI, PANDORA_PKG and PANDORA_OUT are all required")
endif()

get_filename_component(PANDORA_OUT_DIR "${PANDORA_OUT}" DIRECTORY)
file(MAKE_DIRECTORY "${PANDORA_OUT_DIR}")
file(REMOVE "${PANDORA_OUT}")

execute_process(
  COMMAND "${PANDORA_CLI}" package "${PANDORA_PKG}" "${PANDORA_OUT}"
  RESULT_VARIABLE cli_status
  OUTPUT_VARIABLE cli_output
  ERROR_VARIABLE cli_error)

if(NOT cli_status EQUAL 0)
  message(FATAL_ERROR "pandora-cli package failed (${cli_status})\n${cli_output}\n${cli_error}")
endif()

if(NOT EXISTS "${PANDORA_OUT}")
  message(FATAL_ERROR "pandora-cli package reported success but wrote no archive")
endif()

file(READ "${PANDORA_PKG}/sce_sys/param.json" param_text)
string(REGEX MATCH "\"titleId\"[ \t]*:[ \t]*\"([A-Z0-9]+)\"" _ "${param_text}")
set(title_id "${CMAKE_MATCH_1}")
if(NOT title_id)
  message(FATAL_ERROR "no titleId found in ${PANDORA_PKG}/sce_sys/param.json")
endif()

# A ZIP begins with the local file header signature, PK\x03\x04.
file(READ "${PANDORA_OUT}" signature HEX LIMIT 4)
if(NOT signature STREQUAL "504b0304")
  message(FATAL_ERROR "the archive does not start with a local file header: ${signature}")
endif()

# The first stored name starts at byte 30 and must be "<title id>/".
#
# Read as hex: CMake strings stop at a NUL byte, and a ZIP header is full of them.
file(READ "${PANDORA_OUT}" first_name_hex HEX OFFSET 30 LIMIT 16)
string(TOLOWER "${first_name_hex}" first_name_hex)
string(HEX "${title_id}/" expected_hex)
string(TOLOWER "${expected_hex}" expected_hex)
string(LENGTH "${expected_hex}" expected_length)
string(SUBSTRING "${first_name_hex}" 0 ${expected_length} actual_hex)
if(NOT actual_hex STREQUAL expected_hex)
  message(FATAL_ERROR "the first archive entry is ${actual_hex}, expected ${expected_hex} (${title_id}/)")
endif()

# The CLI must also report the hash the catalog needs.
execute_process(
  COMMAND "${PANDORA_CLI}" digest "${PANDORA_OUT}"
  RESULT_VARIABLE digest_status
  OUTPUT_VARIABLE digest_output)

if(NOT digest_status EQUAL 0)
  message(FATAL_ERROR "pandora-cli digest failed (${digest_status})")
endif()

# CMake's regex engine has no {n} intervals, so the length is checked instead.
string(STRIP "${digest_output}" digest_line)
string(REGEX MATCH "^[0-9a-f]+" digest "${digest_line}")
string(LENGTH "${digest}" digest_length)
if(NOT digest_length EQUAL 64)
  message(FATAL_ERROR "digest did not print a SHA-256: ${digest_output}")
endif()

file(SIZE "${PANDORA_OUT}" archive_size)
message(STATUS "packaging ok: ${title_id}, ${archive_size} bytes, sha256 ${digest}")

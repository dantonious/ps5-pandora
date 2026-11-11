cmake_minimum_required(VERSION 3.20)

project(
  ps5-pandora
  VERSION 0.1.0
  DESCRIPTION "All-in-one manager for a jailbroken PS5"
  HOMEPAGE_URL "https://github.com/dantonious/ps5-pandora"
  LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(PANDORA_BUILD_TESTS "Build the test suite" ON)
option(PANDORA_BUILD_CONSOLE "Build for a real PS5 instead of the host" OFF)
option(PANDORA_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

# ---------------------------------------------------------------------------
# Warnings
#
# Warnings are errors by default. The codebase is small enough that keeping it
# warning-free is cheaper than reviewing warnings later, and a compiler warning
# is usually a defect that has not happened yet.
# ---------------------------------------------------------------------------
add_library(pandora-warnings INTERFACE)
if(MSVC)
  target_compile_options(pandora-warnings INTERFACE /W4 /permissive-)
else()
  target_compile_options(pandora-warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wnon-virtual-dtor -Wold-style-cast)
  if(PANDORA_WARNINGS_AS_ERRORS)
    target_compile_options(pandora-warnings INTERFACE -Werror)
  endif()
endif()

# ---------------------------------------------------------------------------
# pandora-core: everything that does not know which platform it runs on
# ---------------------------------------------------------------------------
add_library(pandora-core STATIC
  src/version.cpp
  src/json.cpp
  src/param.cpp
  src/config.cpp
  src/log.cpp
  src/sha256.cpp
  src/zip.cpp)

target_include_directories(pandora-core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

target_compile_definitions(pandora-core PUBLIC
  PANDORA_VERSION="${PROJECT_VERSION}"
  PANDORA_NAME="ps5-pandora")

target_link_libraries(pandora-core PRIVATE pandora-warnings)

if(PANDORA_BUILD_CONSOLE)
  # Only the console build pulls in the payload entry point and libkernel.
  target_sources(pandora-core PRIVATE src/platform_console.cpp)
  target_compile_definitions(pandora-core PUBLIC PANDORA_PLATFORM_CONSOLE)
else()
  target_sources(pandora-core PRIVATE src/platform_host.cpp)
endif()

# ---------------------------------------------------------------------------
# The UI is compiled into the binary. There is no asset directory to lose and no
# file to read at start-up: the document is part of the executable.
# ---------------------------------------------------------------------------
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/ui/index.html" PANDORA_UI_HTML)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ui_index_html.hpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/generated/pandora/ui_index_html.hpp"
  @ONLY)

# ---------------------------------------------------------------------------
# pandora-http-core: routing and the server, separated so the tests can link the
# routing without opening a socket.
# ---------------------------------------------------------------------------
add_library(pandora-http-core STATIC src/http.cpp)
target_link_libraries(pandora-http-core PUBLIC pandora-core)
target_link_libraries(pandora-http-core PRIVATE pandora-warnings)
target_include_directories(pandora-http-core PUBLIC
  "${CMAKE_CURRENT_BINARY_DIR}/generated")

add_executable(pandora-http src/main_http.cpp)
target_link_libraries(pandora-http PRIVATE pandora-http-core pandora-warnings)

add_executable(pandora-cli tools/pandora-cli/main.cpp)
target_link_libraries(pandora-cli PRIVATE pandora-core pandora-warnings)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(PANDORA_BUILD_TESTS)
  enable_testing()

  foreach(suite version json param config sha256 zip platform)
    add_executable(${suite}_test tests/${suite}_test.cpp)
    target_link_libraries(${suite}_test PRIVATE pandora-core pandora-warnings)
    target_include_directories(${suite}_test PRIVATE tests)
    add_test(NAME ${suite} COMMAND ${suite}_test)
  endforeach()

  add_executable(http_test tests/http_test.cpp)
  target_link_libraries(http_test PRIVATE pandora-http-core pandora-warnings)
  target_include_directories(http_test PRIVATE tests)
  add_test(NAME http COMMAND http_test)

  # End-to-end: build a package and ask the CLI to read it back.
  add_test(
    NAME packaging
    COMMAND ${CMAKE_COMMAND}
      -DPANDORA_CLI=$<TARGET_FILE:pandora-cli>
      -DPANDORA_PKG=${CMAKE_CURRENT_SOURCE_DIR}/pkg
      -DPANDORA_OUT=${CMAKE_CURRENT_BINARY_DIR}/test-artifacts/PPSA77010.zip
      -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/packaging_test.cmake)
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS pandora-core pandora-http-core ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS pandora-http pandora-cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# ps5-pandora — developer entry points.
# Run `make` with no target to see the list.

BUILD   ?= build
DIST    ?= dist
PARAM   ?= pkg/sce_sys/param.json
TITLE_ID?= PPSA77010
JOBS    ?= $(shell nproc 2>/dev/null || echo 4)

CMAKE         ?= cmake
CLANG_FORMAT  ?= clang-format

FORMAT_FILES = $(wildcard include/pandora/*.hpp) \
               $(wildcard src/*.cpp) \
               $(wildcard tests/*.cpp) \
               $(wildcard tests/support/*.hpp) \
               $(wildcard tools/pandora-cli/*.cpp)

.PHONY: help configure fmt fmt-check lint build test run package doctor ci clean distclean console clean-console

help: ## Show this help
	@grep -hE '^[a-zA-Z_-]+:.*?## ' $(MAKEFILE_LIST) | awk 'BEGIN{FS=":.*?## "}{printf "  %-14s %s\n", $$1, $$2}'

configure: ## Configure the host build
	$(CMAKE) -S . -B $(BUILD)

fmt: ## Format every C++ source in place
	$(CLANG_FORMAT) -i $(FORMAT_FILES)

fmt-check: ## Fail if anything is not formatted (what CI runs)
	$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_FILES)

lint: ## Build with every warning turned into an error
	$(CMAKE) --build $(BUILD) -j$(JOBS)

build: configure ## Build every target
	$(CMAKE) --build $(BUILD) -j$(JOBS)

test: build ## Run the test suite
	ctest --test-dir $(BUILD) --output-on-failure

run: build ## Serve the dashboard on http://127.0.0.1:8080
	./$(BUILD)/pandora-http

package: build ## Build dist/<TITLEID>.zip from pkg/
	./$(BUILD)/pandora-cli package pkg $(DIST)/$(TITLE_ID).zip

doctor: build ## Check the build environment
	./$(BUILD)/pandora-cli doctor

ci: fmt-check build test ## What CI runs
	@echo "all local gates passed"

console: ## Configure the console build (needs PS5_PAYLOAD_SDK; UNVALIDATED)
	$(CMAKE) -S . -B $(BUILD)-console \
	  -DCMAKE_TOOLCHAIN_FILE=cmake/ps5-toolchain.cmake \
	  -DPANDORA_BUILD_CONSOLE=ON \
	  -DPANDORA_BUILD_TESTS=OFF

clean: ## Remove the build directory and dist/
	rm -rf $(BUILD) $(DIST)

clean-console: ## Remove the console build directory
	rm -rf $(BUILD)-console

# ps5-pandora — developer entry points.
# Run `make` with no target to see the list.

CARGO ?= cargo
PARAM ?= pkg/sce_sys/param.json
DIST  ?= dist

.PHONY: help fmt fmt-check lint test build doc package doctor ci clean build-hw

help: ## Show this help
	@grep -hE '^[a-zA-Z_-]+:.*?## ' $(MAKEFILE_LIST) | awk 'BEGIN{FS=":.*?## "}{printf "  %-12s %s\n", $$1, $$2}'

fmt: ## Format the workspace
	$(CARGO) fmt --all

fmt-check: ## Check formatting without changing files
	$(CARGO) fmt --all --check

lint: ## Run clippy, warnings are errors
	$(CARGO) clippy --workspace --all-targets -- -D warnings

test: ## Run the test suite
	$(CARGO) test --workspace

build: ## Build every crate for the host
	$(CARGO) build --workspace

doc: ## Build and open the API docs
	$(CARGO) doc --workspace --no-deps --open

package: ## Build dist/<TITLEID>.zip from pkg/
	$(CARGO) run -p pandora-cli -- package --source pkg --out $(DIST)

doctor: ## Check the build environment
	$(CARGO) run -p pandora-cli -- doctor

ci: fmt-check lint test ## What CI runs
	@echo "all local gates passed"

build-hw: ## Build for the console (not implemented yet)
	@echo "Hardware builds are not wired up yet."
	@echo "The console target is milestone M0 in docs/ROADMAP.md."
	@exit 1

clean: ## Remove build output
	$(CARGO) clean
	rm -rf $(DIST)

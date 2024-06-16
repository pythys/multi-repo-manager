COMPILER ?= clang
GENERATOR ?= "Unix Makefiles"

all: help

.PHONY: clean
clean: ## Clean generated artifacts
	@echo "Cleaning artifacts..."
	@rm -rf .cache build compile_commands.json

ifeq ($(COMPILER),clang)
  CC = clang
  CXX = clang++
else ifeq ($(COMPILER),gcc)
  CC = gcc
  CXX = g++
else
  $(error Unknown compiler: $(COMPILER))
endif

.PHONY: build
build: ## Compile and generate editor artifacts
	@echo "Building project..."
	CXX=$(CXX) CC=$(CC) cmake -G $(GENERATOR) -B build -S .
	cmake --build build -j $(shell nproc)
	ln -sf build/compile_commands.json compile_commands.json

.PHONY: test
test: build ## Run all unit tests
	@echo "Running tests..."
	@cd build && ctest

.PHONY: lint
lint: ## Lint source code with cpplint
	@echo "Linting src and tests directories..."
	@cpplint --repository=. --recursive --filter=-legal/copyright,-build/c++11,-build/include_subdir src tests

.PHONY: scan
scan: build ## Apply static analysis on code base
	@scan-build -o build/scan-build-results cmake --build build

.PHONY: watch
watch: ## Cycle of clean test lint
	@echo "Watching file changes..."
	@find . -type f ! -path './build/*' | entr -d make clean test lint scan

.PHONY: package
package: build ## Package code to various formats
	@cd build && cpack

.PHONY: dockerize
dockerize: ## Build docker image "mrm"
	@docker build --no-cache --tag mrm .

.PHONY: install
install: ## Install mrm
	@echo "Installing mrm ..."
	@cp build/mrm/mrm /usr/local/bin/mrm

.PHONY: uninstall
uninstall: ## Uninstall mrm
	@echo "Uninstalling mrm ..."
	@rm /usr/local/bin/mrm

define target_help
	awk 'BEGIN {FS = ":.*## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
endef

.PHONY: help
help: ## Show this help
	@echo "Usage: make [target ...] [COMPILER=compiler] [GENERATOR=generator]"
	@echo ""
	@echo "Targets"
	@echo "-------"
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | $(target_help)
	@echo ""
	@echo "Options"
	@echo "-------"
	@echo "COMPILER: \"clang\", \"gcc\""
	@echo "GENERATOR: \"Unix Makefiles\", \"Ninja\""

COMPILER ?= clang
GENERATOR ?= "Ninja"
SCANMATCH = src/**/*.cpp src/**/*.hpp
TESTTYPE ?=

all: help

define check_bin
    @if ! command -v $(1) > /dev/null; then \
        echo "Command '$(1)' is missing. Please install it to proceed."; \
        exit 1; \
    fi
endef

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
	$(call check_bin, cmake)
	@echo "Building project..."
	@CXX=$(CXX) CC=$(CC) cmake -G $(GENERATOR) -B build -S .
	@cmake --build build -j $(shell nproc)
	@ln -sf build/compile_commands.json compile_commands.json
	@ln -sf ../compile_commands.json build/mrm/compile_commands.json
	@ln -sf ../../compile_commands.json build/mrm/lib/compile_commands.json

.PHONY: test
test: build ## Run all tests
	$(call check_bin, ctest)
	@echo "Running tests..."
	@cd build && ctest $(if $(TESTTYPE),-L $(TESTTYPE))

.PHONY: lint
lint: ## Lint source code with cpplint
	$(call check_bin, cpplint)
	@echo "Linting src and tests directories..."
	@cpplint --repository=. --recursive --config=.cpplintrc src tests

.PHONY: scan
scan: ## Scan source code with clang-tidy
	$(call check_bin, clang-tidy)
	@clang-tidy -p build $(wildcard $(SCANMATCH))

.PHONY: watch
watch: ## Cycle of clean test lint
	$(call check_bin, entr)
	@echo "Watching file changes..."
	@find . -type f ! -path './build/*' | entr -d make clean test lint

.PHONY: install
install: ## Install mrm
	@echo "Installing mrm ..."
	@cp build/mrm/mrm /usr/local/bin/mrm

.PHONY: uninstall
uninstall: ## Uninstall mrm
	@echo "Uninstalling mrm ..."
	@rm /usr/local/bin/mrm

.PHONY: completion
completion: ## Generate shell completion scripts
	$(call check_bin, complgen)
	@echo "Generating completion scripts..."
	@mkdir -p build/completions
	@complgen aot mrm.usage --bash-script build/completions/mrm-completions.sh
	@complgen aot mrm.usage --zsh-script build/completions/_mrm

.PHONY: package
package: build ## Package code to various formats
	$(call check_bin, cpack)
	@cd build && cpack

.PHONY: dockerize
dockerize: ## Build docker image "mrm"
	$(call check_bin, docker)
	@docker build --platform=linux/amd64 --no-cache --tag mrm .

define target_help
	awk 'BEGIN {FS = ":.*## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'
endef

.PHONY: help
help: ## Show this help
	@echo "Usage: make [target...] [option...]"
	@echo ""
	@echo "Targets"
	@echo "-------"
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | $(target_help)
	@echo ""
	@echo "Options"
	@echo "-------"
	@echo "COMPILER:            \"clang\", \"gcc\""
	@echo "GENERATOR:           \"Ninja\", \"Unix Makefiles\""
	@echo "SCANMATCH:           glob-pattern-here (e.g. src/**/*.cpp)"
	@echo "TESTTYPE:            \"unit\", \"integration\" (empty for all)"

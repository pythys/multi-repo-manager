#CC ?= clang
#CXX ?= clang++
#GENERATOR ?= Ninja
GENERATOR ?= "Unix Makefiles"
CC ?= gcc
CXX ?= g++

all: help

.PHONY: clean
clean: ## Clean generated artifacts
	@echo "Cleaning artifacts..."
	@rm -rf .cache build compile_commands.json

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

.PHONY: watch
watch: ## Cycle of clean test lint
	@echo "Watching file changes..."
	@find . -type f ! -path './build/*' | entr -d make clean test lint

.PHONY: scan
scan: build ## Apply static analysis on code base
	@scan-build -o build/scan-build-results cmake --build build

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

define display_help
	awk 'BEGIN {FS = ":.*## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
endef

.PHONY: help
help: ## Show this help
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | $(display_help)

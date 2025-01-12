COMPILER ?= clang
GENERATOR ?= "Ninja"
SCANMATCH = src/**/*.cpp src/**/*.hpp

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
	@CXX=$(CXX) CC=$(CC) cmake -G $(GENERATOR) -B build -S .
	@cmake --build build -j $(shell nproc)
	@ln -sf build/compile_commands.json compile_commands.json
	@ln -sf ../compile_commands.json build/mrm/compile_commands.json
	@ln -sf ../../compile_commands.json build/mrm/lib/compile_commands.json

.PHONY: test
test: build ## Run all unit tests
	@echo "Running tests..."
	@cd build && ctest

.PHONY: lint
lint: ## Lint source code with cpplint
	@echo "Linting src and tests directories..."
	@cpplint --repository=. --recursive --config=.cpplintrc src tests

.PHONY: scan
scan: ## Scan source code with clang-tidy
	@clang-tidy -p build $(wildcard $(SCANMATCH))

.PHONY: watch
watch: ## Cycle of clean test lint
	@echo "Watching file changes..."
	@find . -type f ! -path './build/*' | entr -d make clean test lint

.PHONY: package
package: build ## Package code to various formats
	@cd build && cpack

.PHONY: dockerize
dockerize: ## Build docker image "mrm"
	@docker build --platform=linux/amd64 --no-cache --tag mrm .

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
	@echo "Usage: make [target...] [options...]"
	@echo ""
	@echo "Targets"
	@echo "-------"
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | $(target_help)
	@echo ""
	@echo "Options"
	@echo "-------"
	@echo "COMPILER: \"clang\", \"gcc\""
	@echo "GENERATOR: \"Ninja\", \"Unix Makefiles\""
	@echo "SCANMATCH: glob-pattern-here e.g. src/**/*.cpp"

VCPKG_ROOT := $(shell echo $$VCPKG_ROOT)
VCPKG_CMAKE := $(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake

all: help

define display_help
	awk 'BEGIN {FS = ":.*## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
endef

help: ## Show this help
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | $(display_help)

clean: ## Clean generated artifacts
	@echo "Cleaning artifacts..."
	@rm -rf .cache build compile_commands.json

build: ## Compile and generate editor artifacts
	@echo "Building project..."
ifdef VCPKG_ROOT
	cmake -B build -S . -D CMAKE_TOOLCHAIN_FILE=$(VCPKG_CMAKE) -DCMAKE_EXPORT_COMPILE_COMMANDS=YES
	cmake --build build -j $(shell nproc)
	ln -sf build/compile_commands.json compile_commands.json
else
	$(error VCPKG_ROOT environment variable is not set)
endif

test: build ## Run all unit tests
	@echo "Running tests..."
	@cd build && ctest

lint: ## Lint source code with cpplint
	@echo "Linting src and tests directories..."
	@cpplint --repository=. --recursive --filter=-legal/copyright,-build/c++11,-build/include_subdir src tests

package: build ## Package code to various formats
	@cd build && cpack

watch: ## Cycle of clean test lint
	@echo "Watching file changes..."
	@find . -type f ! -path './build/*' | entr -d make clean test lint

.PHONY: help clean build test lint package watch


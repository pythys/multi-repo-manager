COMPILER ?= clang
GENERATOR ?= "Ninja"
SCANMATCH = src/**/*.cpp src/**/*.hpp
TESTTYPE ?=
VCPKG_ROOT ?=

all: help

define check_bin
    @if ! command -v $(1) > /dev/null; then \
        echo "Command '$(1)' is missing. Please install it to proceed."; \
        exit 1; \
    fi
endef

ifeq ($(strip $(VCPKG_ROOT)),)
    VCPKG_TOOLCHAIN :=
else
    VCPKG_TOOLCHAIN := -DCMAKE_TOOLCHAIN_FILE=$(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake
endif

ifeq ($(COMPILER),clang)
    CC = clang
    CXX = clang++
else ifeq ($(COMPILER),gcc)
    CC = gcc
    CXX = g++
else
    $(error Unknown compiler: $(COMPILER))
endif

.PHONY: clean
clean: ## Clean generated artifacts
	@echo "Cleaning artifacts..."
	@rm -rf .cache build compile_commands.json

.PHONY: build
build: ## Compile and generate editor artifacts
	$(call check_bin, cmake)
	@echo "Building project..."
	@CXX=$(CXX) CC=$(CC) cmake -G "$(GENERATOR)" -B build -S . $(VCPKG_TOOLCHAIN)
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
lint: ## Lint with clang-format and report issues
	$(call check_bin, clang-format)
	@find src tests \( -name "*.cpp" -o -name "*.hpp" \) \
		-exec clang-format -Werror -i --dry-run {} \;

.PHONY: lint-fix
lint-fix: ## Lint and automatically fix formatting issues
	@find src tests \( -name "*.cpp" -o -name "*.hpp" \) \
		-exec clang-format -Werror -i {} \;
	@find . -name "CMakeLists.txt" \
		-not -path "./build/*" \
		-exec cmake-format -i {} +

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

.PHONY: docs
docs: ## Generate doxygen documentation
	$(call check_bin, doxygen)
	@doxygen

.PHONY: completion
completion: ## Generate shell completion scripts
	$(call check_bin, complgen)
	@echo "Generating completion scripts..."
	@mkdir -p build/completions
	@complgen --bash build/completions/mrm-completions.sh mrm.usage
	@complgen --zsh build/completions/_mrm mrm.usage

.PHONY: package
package: build ## Package code to various formats
	$(call check_bin, cpack)
	@cd build && cpack

.PHONY: dockerize
dockerize: ## Build docker image "mrm"
	$(call check_bin, docker)
	@docker build --platform=linux/amd64 --no-cache --tag mrm .

define target_help
	awk 'BEGIN {FS = ":.*## "}; {printf "\033[36m%-16s\033[0m %s\n", $$1, $$2}'
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
	@echo "COMPILER:        \"clang\", \"gcc\" - default: \"clang\""
	@echo "GENERATOR:       \"Ninja\", \"Unix Makefiles\" - default: \"Ninja\""
	@echo "SCANMATCH:       <glob-pattern> - default: \"src/**/*.cpp src/**/*.hpp\""
	@echo "TESTTYPE:        \"unit\", \"integration\" - default: all tests"
	@echo "VCPKG_ROOT:      \"/path/to/vcpkg/\" - empty to disable vcpkg"

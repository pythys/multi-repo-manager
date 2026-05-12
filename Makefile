COMPILER ?= clang
GENERATOR ?= "Ninja"
BUILDTYPE ?= Release
SCANFILTER ?= (src|tests)/.*\.cpp$$
TESTTYPE ?=
NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

all: help

define check_bin
    @if ! command -v $(1) > /dev/null; then \
        echo "Command '$(1)' is missing. Please install it to proceed."; \
        exit 1; \
    fi
endef

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
	@rm -rf .cache build
	@rm -f compile_commands.json CMakeUserPresets.json

.PHONY: deps
deps: ## Install dependencies with Conan
	$(call check_bin, conan)
	@if ! conan profile show >/dev/null 2>&1; then \
		echo "No Conan profile found, creating default profile..."; \
		conan profile detect --force; \
	fi
	@echo "Installing dependencies with Conan..."
	@mkdir -p build/conan
	@conan install . --build=missing -of build/conan -s build_type=$(BUILDTYPE)

.PHONY: build
build: deps ## Compile and generate artifacts
	$(call check_bin, cmake)
	@echo "Building project..."
	@CXX=$(CXX) CC=$(CC) cmake -G "$(GENERATOR)" -B build -S . \
		-DCMAKE_TOOLCHAIN_FILE=build/conan/conan_toolchain.cmake \
		-DCMAKE_BUILD_TYPE=$(BUILDTYPE)
	@cmake --build build -j $(NPROC)
	@ln -sf build/compile_commands.json compile_commands.json

.PHONY: test
test: build ## Run all tests
	$(call check_bin, ctest)
	@echo "Running tests..."
	@cd build && ctest $(if $(TESTTYPE),-L $(TESTTYPE))

.PHONY: lint
lint: ## Lint and report issues
	$(call check_bin, clang-format)
	$(call check_bin, cmake-format)
	@find src tests \( -name "*.cpp" -o -name "*.hpp" \) \
		-exec clang-format -Werror -i --dry-run {} \;
	@failed=0; \
	for file in $$(find . \( -name "CMakeLists.txt" -o -name "*.cmake" \) \
		-not -path "./build/*"); do \
		if ! cmake-format --check "$$file" >/dev/null 2>&1; then \
			echo "$$file: formatting differs (cmake-format)"; \
			failed=1; \
		fi; \
	done; \
	if [ $$failed -ne 0 ]; then \
		exit 1; \
	fi

.PHONY: lint-fix
lint-fix: ## Lint and automatically fix issues
	$(call check_bin, clang-format)
	$(call check_bin, cmake-format)
	@find src tests \( -name "*.cpp" -o -name "*.hpp" \) \
		-exec clang-format -Werror -i {} \;
	@find . \( -name "CMakeLists.txt" -o -name "*.cmake" \) \
		-not -path "./build/*" \
		-exec cmake-format -i {} +

.PHONY: scan
scan: ## Scan source code with clang-tidy
	$(call check_bin, run-clang-tidy)
	@run-clang-tidy -p build -quiet -j $(NPROC) \
		-header-filter='(src|tests)/.*\.hpp$$' \
		'$(SCANFILTER)'

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
docs: ## Generate documentation
	$(call check_bin, zensical)
	@echo "Generating docs/index.md from README.md..."
	@sed 's|docs/||g' README.md > docs/index.md
	@zensical build

.PHONY: package
package: build ## Package code to various formats
	$(call check_bin, cpack)
	@cd build && cpack

.PHONY: dockerize
dockerize: ## Build docker image "mrm"
	$(call check_bin, docker)
	@docker build --platform=linux/amd64 --no-cache --tag mrm .

.PHONY: version
version: ## Update version across files (VERSION=1.2.3)
	@if [ -n "$(VERSION)" ]; then \
		echo "$(VERSION)" > VERSION; \
	fi
	@version=$$(cat VERSION | tr -d '[:space:]'); \
	sh scripts/update-version.sh "$$version"

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
	@echo "BUILDTYPE:       \"Debug\", \"Release\" - default: \"Release\""
	@echo "COMPILER:        \"clang\", \"gcc\" - default: \"clang\""
	@echo "GENERATOR:       \"Ninja\", \"Unix Makefiles\" - default: \"Ninja\""
	@echo "SCANFILTER:      <regex> - default: \"(src|tests)/.*\\.cpp\$$\""
	@echo "TESTTYPE:        \"unit\", \"integration\" - default: all tests"
	@echo "VERSION:         <version> - update VERSION file before propagating"

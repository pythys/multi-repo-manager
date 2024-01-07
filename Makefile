VCPKG_ROOT := $(shell echo $$VCPKG_ROOT)
VCPKG_CMAKE := $(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake

all: build

clean:
	@echo "Cleaning artifacts..."
	rm -rf .cache build compile_commands.json

build:
	@echo "Building project..."
ifdef VCPKG_ROOT
	cmake -B build -S . -D CMAKE_TOOLCHAIN_FILE=$(VCPKG_CMAKE) -DCMAKE_EXPORT_COMPILE_COMMANDS=YES
	cmake --build build -j $(shell nproc)
	ln -sf build/compile_commands.json compile_commands.json
else
	$(error VCPKG_ROOT environment variable is not set)
endif

test: build
	@echo "Running tests..."
	cd build && ctest

watch:
	@echo "Watching file changes..."
	find . -type f ! -path './build/*' | entr -d make clean test lint

lint:
	@echo "Linting src and tests directories..."
	cpplint --repository=. --recursive --filter=-legal/copyright,-build/c++11,-build/include_subdir src tests

package: build
	cd build && cpack

.PHONY: clean build test watch lint package


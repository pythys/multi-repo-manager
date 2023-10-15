#!/usr/bin/env python
import subprocess
import os
import sys

def show_help():
    print("Usage: build.py [OPTIONS] [TASKS]")
    print("\nAvailable options:")
    print("  -h, --help   Show this help message and exit.")
    print("\nAvailable tasks:")
    print("  clean        Clean generated artifacts.")
    print("  build        Build the project.")
    print("  test         Run all tests.")
    print("  watch        Watch file changes to rebuild.")
    print("  package      Package the project.")

def clean():
    print("Cleaning build directory...")
    subprocess.run(["rm", "-rf", "build"])
    subprocess.run(["rm", "TAGS"])

def build():
    print("Building project...")
    vcpkg_root = os.environ.get('VCPKG_ROOT')
    if vcpkg_root:
        vcpkg_cmake_str = f"{vcpkg_root}/scripts/buildsystems/vcpkg.cmake"
        subprocess.run(["cmake", "-B", "build", "-S", ".", "-D", f"CMAKE_TOOLCHAIN_FILE={vcpkg_cmake_str}"])
        subprocess.run(["cmake", "--build", "build", "-j", str(os.cpu_count())])
    else:
        raise EnvironmentError("VCPKG_ROOT environment variable is not set.")

def test():
    subprocess.run(["ctest"], cwd="build")

def watch():
    print("Watching file changes...")
    try:
        subprocess.run(["find . -type f ! -path './build/*' | entr -d ./build.py test"], shell=True)
    except KeyboardInterrupt:
        print("Stopped watching for changes.")

def tags():
    print("Generating emacs ctags...")
    subprocess.run(["ctags", "-e", "-R"])

def package():
    print("Packaging project...")
    subprocess.run(["cpack"], cwd="build")

task_dependencies = {
    'build': [],
    'test': ['build'],
    'watch': [],
    'package': ['build'],
}

def execute_task(task_name, executed=set()):
    if task_name in executed:
        return

    for dependency in task_dependencies.get(task_name, []):
        execute_task(dependency, executed)

    print(f"Executing task: {task_name}")
    globals()[task_name]()
    executed.add(task_name)

if __name__ == "__main__":
    tasks = sys.argv[1:]
    if not tasks or '-h' in tasks or '--help' in tasks:
        show_help()
    else:
        for task in tasks:
            execute_task(task)

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
    print("  lint         Lint with cpplint.")
    print("  emacs        Generate emacs artifacts")
    print("  package      Package the project.")

def clean():
    print("Cleaning artifacts...")
    subprocess.run(["rm", "-rf",
        ".cache",
        "TAGS",
        "build",
        "compile_commands.json"
    ])

def build():
    print("Building project...")
    vcpkg_root = os.environ.get('VCPKG_ROOT')
    if vcpkg_root:
        vcpkg_cmake_str = f"{vcpkg_root}/scripts/buildsystems/vcpkg.cmake"
        subprocess.run(["cmake",
            "-B", "build",
            "-S", ".",
            "-D", f"CMAKE_TOOLCHAIN_FILE={vcpkg_cmake_str}",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=YES"
        ])
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

def lint():
    directories = ["src", "tests"]
    for directory in directories:
        print(f"Linting {directory} directory...")
        subprocess.run([
            "cpplint",
            "--repository=.",
            "--recursive",
            "--filter=-legal/copyright,-build/c++11,-build/include_subdir",
            directory
        ])

def emacs():
    print("Linking compile_commands.json...")
    subprocess.run(["ln", "-sf", "build/compile_commands.json", "compile_commands.json"])

def package():
    print("Packaging project...")
    subprocess.run(["cpack"], cwd="build")

task_dependencies = {
    'build': [],
    'test': ['build'],
    'emacs': ['build'],
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

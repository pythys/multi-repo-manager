from conan import ConanFile

class MrmConan(ConanFile):
    name = "mrm"
    version = "0.1.0"
    package_type = "application"
    settings = "os", "compiler", "build_type", "arch"
    requires = (
        "boost/1.90.0",
        "cli11/2.6.0",
        "ftxui/6.1.9",
        "gtest/1.17.0",
        "libgit2/1.9.1",
        "yaml-cpp/0.8.0",
    )
    generators = "CMakeDeps", "CMakeToolchain"

    def configure(self):
        self.options["*"].shared = False
        self.options["boost"].without_cobalt = True # recipe bug #29193 workaround
        if self.settings.os == "Macos": # macos ssl workaround
            self.options["libgit2"].with_https = "security"

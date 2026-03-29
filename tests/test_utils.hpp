#ifndef TESTS_TEST_UTILS_HPP_
#define TESTS_TEST_UTILS_HPP_

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace test_utils {

namespace fs = std::filesystem;

class TempDir {
  public:
    explicit TempDir(const std::string &prefix = "mrm-test") {
        const auto unique = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        path_ = fs::temp_directory_path() / (prefix + "-" + unique);
        fs::create_directories(path_);
    }

    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }

    TempDir(const TempDir &) = delete;
    TempDir &operator=(const TempDir &) = delete;

    const fs::path &path() const {
        return path_;
    }

  private:
    fs::path path_;
};

class CurrentPathGuard {
  public:
    CurrentPathGuard() : saved_(fs::current_path()) {}

    ~CurrentPathGuard() {
        std::error_code ec;
        fs::current_path(saved_, ec);
    }

    CurrentPathGuard(const CurrentPathGuard &) = delete;
    CurrentPathGuard &operator=(const CurrentPathGuard &) = delete;

  private:
    fs::path saved_;
};

inline std::string read_file(const fs::path &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    return std::string(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
}

inline void write_file(const fs::path &path, const std::string &content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to write file: " + path.string());
    }
    file << content;
}

inline bool contains(const std::string &haystack, const std::string &needle) {
    return haystack.find(needle) != std::string::npos;
}

} // namespace test_utils

#endif // TESTS_TEST_UTILS_HPP_

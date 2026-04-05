#include "util/common.hpp"
#include "util/constants.hpp"
#include <fstream>
#include <stdexcept>

std::string
replace_all(std::string str, std::string_view from, std::string_view to) {
    if (from.empty()) {
        return str;
    }
    std::size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.size(), to);
        pos += to.size();
    }
    return str;
}

std::string
construct_repo_path(const std::string &root, const std::string &name) {
    return (std::filesystem::path(root) / name).string();
}

void write_file(const std::string &path, const std::string &content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + path);
    }
    file << content;
    if (file.fail()) {
        throw std::runtime_error("Failed to write to file: " + path);
    }
}

boost::asio::thread_pool create_thread_pool(int jobs) {
    const auto effective_size =
        static_cast<std::size_t>(jobs > 0 ? jobs : SYNC_POOL_SIZE);
    return boost::asio::thread_pool(effective_size);
}

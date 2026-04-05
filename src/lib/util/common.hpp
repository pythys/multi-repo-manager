#ifndef SRC_LIB_UTIL_COMMON_HPP_
#define SRC_LIB_UTIL_COMMON_HPP_

#include <boost/asio/thread_pool.hpp>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>

template <typename Container>
std::string join(const Container &items, std::string_view separator) {
    std::ostringstream out;
    bool first = true;
    for (const auto &item : items) {
        if (!first) {
            out << separator;
        }
        out << item;
        first = false;
    }
    return out.str();
}

std::string
replace_all(std::string str, std::string_view from, std::string_view to);

std::string
construct_repo_path(const std::string &root, const std::string &name);

void write_file(const std::string &path, const std::string &content);

boost::asio::thread_pool create_thread_pool(int jobs);

#endif // SRC_LIB_UTIL_COMMON_HPP_

#ifndef SRC_LIB_COMPLETION_COMPLETION_BASH_HPP_
#define SRC_LIB_COMPLETION_COMPLETION_BASH_HPP_

#include "command/completion.hpp"
#include <string>

std::string render_bash(const CompletionSpec &spec);

#endif // SRC_LIB_COMPLETION_COMPLETION_BASH_HPP_

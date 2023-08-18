#include "logging.hpp"

std::function<void(std::string const&)> s_error_logger;
std::function<void(std::string const&)> s_logger;

[[maybe_unused]] void set_info_logger(std::function<void(std::string const&)> const &logger) {
    s_logger = logger;
    if (!s_error_logger)
        s_error_logger = logger;
}

[[maybe_unused]] void set_error_logger(std::function<void(std::string const&)> const &logger) {
    s_error_logger = logger;
}

bool has_logger() {
    return s_logger != nullptr;
}

bool has_error_logger() {
    return s_error_logger != nullptr;
}
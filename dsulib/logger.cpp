#include "logger.hpp"

std::function<void(std::string const&)> g_logger = [](auto&){};

void set_logger(std::function<void(std::string const&)> const& logger) {
    g_logger = logger;
}
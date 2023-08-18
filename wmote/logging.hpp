#pragma once
#include <functional>
#include <string_view>

[[maybe_unused]] void set_info_logger(std::function<void(std::string const&)> const& logger);

[[maybe_unused]] void set_error_logger(std::function<void(std::string const&)> const& logger);

bool has_logger();
bool has_error_logger();

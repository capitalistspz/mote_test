#pragma once
#include <functional>
#include <string>
#include <fmt/format.h>

extern std::function<void(std::string const&)> g_logger;

template <typename ...V>
constexpr void log_info(fmt::format_string<V...> fmt, V &&... v) {
    g_logger(fmt::format(fmt, std::forward<V>(v)...));
}
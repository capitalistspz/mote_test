#pragma once

#include <functional>
#include <ranges>

#include "logging.hpp"

#if true
#include <fmt/format.h>
template <typename ...V>
using wm_format_string = fmt::format_string<V...>;
template <typename... Args>
auto wm_format(Args&&... args) -> decltype(fmt::format(std::forward<Args>(args)...)) {
    return fmt::format(std::forward<Args>(args)...);
}
#else
#include <format>

template <typename Char, typename Element>
using range_formatter_type =
        std::formatter<std::remove_cvref_t<Element>,
        Char>;

template<typename T, class CharT> requires std::same_as<T, std::remove_cvref_t<T>> && requires (T& t) {
    std::formatter<T,CharT>{};
}
struct std::formatter<T, CharT> {
        template <typename Ctx>
    constexpr auto parse(Ctx &ctx) -> decltype(ctx.begin()){
        auto it = ctx.begin();
        auto end = ctx.end();


        if (it != end && *it != '}') {
            if (*it != ':') throw std::format_error("invalid format specifier");
            ++it;
        }
        ctx.advance_to(it);

        return underlying.parse(ctx);
    }

    template <typename Range, typename Ctx>
    auto format(Range&& view, Ctx &fc) const -> decltype(fc.out()) {
        auto out = std::copy(opening_bracket.begin(), opening_bracket.end(), fc.out());
        if (!std::ranges::empty(view)){
            auto it = std::ranges::begin(view);
            auto end = std::ranges::end(view);

            fc.advance_to(out);
            out = underlying.format(*it++, out);
            for (; it != end; ++it) {
                out = std::copy(separator.begin(), separator.end(), out);
                fc.advance_to(out);
                out = underlying.format(*it, out);
            }
        }
        out = std::copy(closing_bracket.begin(), closing_bracket.end(), fc.out());


        return out;
    }

private:
    std::basic_string<CharT> opening_bracket = "[";
    std::basic_string<CharT> closing_bracket = "]";
    std::basic_string<CharT> separator = ",";
    range_formatter_type<CharT, T> underlying;
};
template <typename ...V>
using wm_format_string = std::format_string<V...>;

template<typename... Args>
constexpr auto wm_format(Args &&... args) -> decltype(std::format(std::forward<Args>(args)...)) {
    return std::format(std::forward<Args>(args)...);
}
#endif

extern std::function<void(std::string)> s_error_logger;
extern std::function<void(std::string)> s_logger;

template<typename ...V>
constexpr void log_info(wm_format_string<V...> fmt, V &&... v) {
    s_logger(wm_format(fmt, std::forward<V>(v)...));
}

template<typename ...V>
constexpr void log_error(wm_format_string<V...> fmt, V &&... v) {
    s_error_logger(wm_format(fmt, std::forward<V>(v)...));
}

void log_info(std::string const &message);

void log_error(std::string const &message);



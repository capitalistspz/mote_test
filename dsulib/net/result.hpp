
#pragma once

#include <system_error>
#include <stdexcept>
#include <variant>

template<typename ValueType>
class result {
private:
    explicit result(std::error_code ec)
            : m_value(ec) {
    }

public:
    result(ValueType value)
            : m_value(value) {
    }

    static result error(std::error_code ec) {
        return result(ec);
    }

    [[nodiscard]] bool has_value() const {
        return std::holds_alternative<ValueType>(m_value);
    }

    [[nodiscard]] std::error_code error() const {
        if (std::holds_alternative<std::error_code>(m_value))
            return std::get<std::error_code>(m_value);
        return {};
    }

    decltype(auto) operator*() const {
        return std::get<ValueType>(m_value);
    }
    decltype(auto) operator*() {
        return std::get<ValueType>(m_value);
    }

    decltype(auto) operator->(){
        return &std::get<ValueType>(m_value);
    }
    decltype(auto) operator->() const {
        return &std::get<ValueType>(m_value);
    }

    decltype(auto) value() {
        return std::get<ValueType>(m_value);
    }
    decltype(auto) value() const {
        return std::get<ValueType>(m_value);
    }

    operator bool() const {
        return std::holds_alternative<ValueType>(m_value);
    }

private:
    std::variant<ValueType, std::error_code> m_value;

};


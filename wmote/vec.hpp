#pragma once

#include <array>
#include <cstddef>


template<typename T>
concept number_type = std::is_integral_v<T> || std::is_floating_point_v<T>;

template<number_type NumberType = double>
struct vec3 {
    template<number_type T>
    vec3(vec3<T> const &val)
            : x(static_cast<NumberType>(val.x)), y(static_cast<NumberType>(val.y)), z(static_cast<NumberType>(val.z)) {}

    vec3(NumberType X, NumberType Y, NumberType Z) : x(X), y(Y), z(Z) {}
    vec3() : x(0), y(0), z(0) {}
    NumberType x;
    NumberType y;
    NumberType z;
};

template<number_type T>
// Returns a vector containing the minimum values for each axis
vec3<T> min(vec3<T> a, vec3<T> b){
    return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)};
}
template<number_type T>
// Returns a vector containing the maximum values for each axis
vec3<T> max(vec3<T> a, vec3<T> b){
    return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)};
}

template<number_type NumberType = double>
struct vec2 {
    template<number_type T>
    vec2(vec2<T> const &val)
            : x(static_cast<NumberType>(val.x)), y(static_cast<NumberType>(val.y)){}

    vec2(NumberType X, NumberType Y) : x(X), y(Y) {}
    vec2() : x(0), y(0) {}

    NumberType x;
    NumberType y;
};

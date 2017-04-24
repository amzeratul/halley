#pragma once
#include <tuple>

/*
 * Implements various type trait objects
 */

namespace sel {
namespace detail {

template <typename T>
struct _arity {
    static constexpr int value = 1;
};

template <typename... Vs>
struct _arity<std::tuple<Vs...>> {
    static constexpr int value = sizeof...(Vs);
};

template <>
struct _arity<void> {
    static constexpr int value = 0;
};

template <std::size_t... Is>
struct _indices {};

template <std::size_t N, std::size_t... Is>
struct _indices_builder : _indices_builder<N-1, N-1, Is...> {};

template <std::size_t... Is>
struct _indices_builder<0, Is...> {
    using type = _indices<Is...>;
};

template <typename T> struct _id {};
}
}

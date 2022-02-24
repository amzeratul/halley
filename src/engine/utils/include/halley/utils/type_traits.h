#pragma once

#include <type_traits>

namespace Halley
{
	// is_detected_v is based on https://en.cppreference.com/w/cpp/experimental/is_detected

	namespace detail {
	    template<template<class...> class Expr, class... Args>
	    std::false_type is_detected_impl(...);

	    template<template<class...> class Expr, class... Args>
	    std::true_type is_detected_impl(std::void_t<Expr<Args...>>*);
	}

	template<template<class...> class Expr, class... Args>
	using is_detected = decltype(detail::is_detected_impl<Expr, Args...>(nullptr));

	template<template<class...> class Expr, class... Args>
	constexpr bool is_detected_v = is_detected<Expr, Args...>::value;
}

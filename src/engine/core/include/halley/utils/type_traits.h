#pragma once

#include <type_traits>

namespace Halley
{
	// is_detected_v is based on https://en.cppreference.com/w/cpp/experimental/is_detected

	struct nonesuch {
	    ~nonesuch() = delete;
	    nonesuch(nonesuch const&) = delete;
	    void operator=(nonesuch const&) = delete;
	};

	namespace detail {
		template <class Default, class AlwaysVoid, template<class...> class Op, class... Args>
		struct detector {
			using value_t = std::false_type;
			using type = Default;
		};
		 
		template <class Default, template<class...> class Op, class... Args>
		struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
			using value_t = std::true_type;
			using type = Op<Args...>;
		};
	}
	 
	template <template<class...> class Op, class... Args>
	using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;
	
	template<template<class...> class Expr, class... Args>
	constexpr bool is_detected_v = is_detected<Expr, Args...>::value;
}

#pragma once

#include <optional>

namespace Halley
{
	template <typename T>
	using Maybe = std::optional<T>;

	template <typename T>
	using MaybeRefWrap = std::optional<std::reference_wrapper<T>>;
}

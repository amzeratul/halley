#pragma once

#include <array>
#include <functional>
#include <cassert>
#include "halley/support/exception.h"
#include <optional>

namespace Halley
{
	template <typename T>
	using Maybe = std::optional<T>;
}

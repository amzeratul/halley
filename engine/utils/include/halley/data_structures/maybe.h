#pragma once

#include <array>
#include <functional>
#include <cassert>
#include "halley/support/exception.h"
#include <boost/optional.hpp>

namespace Halley
{
	template <typename T>
	using Maybe = boost::optional<T>;
}

#pragma once

#include <vector>

namespace Halley {
	template<typename T> using Vector = std::vector<T, std::allocator<T>>;
}

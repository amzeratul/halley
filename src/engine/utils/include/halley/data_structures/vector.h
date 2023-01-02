#pragma once

#if HAS_EASTL

#include <EASTL/vector.h>

namespace Halley {
	template<typename T> using Vector = eastl::vector<T>;
}

#else

#include <vector>
#include "vector_size32.h"

namespace Halley
{
	template<typename T, typename Allocator = std::allocator<T>, int Padding = 0>
	using Vector = VectorSize32<T, Allocator, Padding>;
}

#endif

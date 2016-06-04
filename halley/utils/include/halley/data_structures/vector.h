#pragma once

#if HAS_EASTL

#include <EASTL/vector.h>

namespace Halley {
	template<typename T> using Vector = eastl::vector<T>;
}

#else

#include <vector>

namespace Halley
{
	template<typename T> using Vector = std::vector<T>;
}

#endif

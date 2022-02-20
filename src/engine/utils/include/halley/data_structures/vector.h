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
	template<typename T> using Vector = std::vector<T>;
	//template<typename T> using Vector = VectorSize32<T>;
}

#endif

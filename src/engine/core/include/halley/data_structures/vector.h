#pragma once
#include "temp_allocator.h"

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
	template<typename T, typename Allocator = std::allocator<T>, int Padding = 0, bool EnableSBO = true>
	using Vector = VectorSize32<T, Allocator, Padding, EnableSBO>;

	template<typename T>
	using VectorTemp = VectorSize32<T, TempPoolAllocator<T>, 0, false>;

	using Byte = unsigned char;
	using Bytes = Vector<Byte>;
}

#endif

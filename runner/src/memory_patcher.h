#pragma once
#include <vector>
#include "symbol_loader.h"

namespace Halley
{
	class MemoryPatchingMappings
	{
	public:
		std::vector<void*> src;
		std::vector<void*> dst;
		void* minSrc;
		void* maxSrc;

		void generate(const std::vector<DebugSymbol>& prev, const std::vector<DebugSymbol>& next);
	};

	class MemoryPatcher
	{
	public:
		static void patch(MemoryPatchingMappings& mappings);
	};
}

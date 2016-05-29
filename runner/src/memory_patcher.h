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
		std::vector<std::string> name;
		void* minSrc;
		void* maxSrc;

		void generate(const std::vector<DebugSymbol>& prev, const std::vector<DebugSymbol>& next);
	};

	class MemoryPatcher
	{
	public:
		static void patch(const MemoryPatchingMappings& mappings);

	private:
		static size_t patchMemory(void* address, size_t len, const MemoryPatchingMappings& mappings);
	};
}

#pragma once
#include <halley/data_structures/vector.h>
#include "symbol_loader.h"

namespace Halley
{
	class MemoryPatchingMappings
	{
	public:
		Vector<void*> src;
		Vector<void*> dst;
		Vector<std::string> name;
		void* minSrc;
		void* maxSrc;

		void generate(const Vector<DebugSymbol>& prev, const Vector<DebugSymbol>& next);
	};

	class MemoryPatcher
	{
	public:
		static void patch(const MemoryPatchingMappings& mappings);

	private:
		static size_t patchMemory(void* address, size_t len, const MemoryPatchingMappings& mappings);
	};
}

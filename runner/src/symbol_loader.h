#pragma once
#include <halley/text/halleystring.h>

namespace Halley
{
	class DebugSymbol
	{
	public:
		String name;
		void* address;
		size_t size;
	};

	class SymbolLoader
	{
	public:
		static std::vector<DebugSymbol> loadSymbols(String debugFile);
	};
}

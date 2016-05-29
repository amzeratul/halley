#pragma once
#include <halley/text/halleystring.h>

namespace Halley
{
	class DynamicLibrary;

	class DebugSymbol
	{
	public:
		std::string name;
		void* address;
		size_t size;

		DebugSymbol(std::string name, void* address, size_t size);
	};

	class SymbolLoader
	{
	public:
		static std::vector<DebugSymbol> loadSymbols(DynamicLibrary& dll);
	};
}

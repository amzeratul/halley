#pragma once
#include <halley/text/halleystring.h>

namespace Halley
{
	class DynamicLibrary;

	class DebugSymbol
	{
	public:
		DebugSymbol(std::string name, void* address, size_t size);
		std::string getName() const { return name; }
		void* getAddress() const;
		size_t getSize() const { return size; }
		void appendToName(size_t id);

	private:
		std::string name;
		size_t address;
		size_t size;
	};

	class SymbolLoader
	{
	public:
		static std::vector<DebugSymbol> loadSymbols(DynamicLibrary& dll);
	};
}

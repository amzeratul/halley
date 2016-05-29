#include "symbol_loader.h"
#include <halley/support/exception.h>
#include <iostream>
#include "dynamic_library.h"

using namespace Halley;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")

static BOOL CALLBACK loadSymbolsCallback(PSYMBOL_INFO symInfo, unsigned long symbolSize, void* userContext)
{
	auto& symbols = *reinterpret_cast<std::vector<DebugSymbol>*>(userContext);
	
	symbols.push_back(DebugSymbol(symInfo->Name, reinterpret_cast<void*>(symInfo->Address), sizeof(void*)));

	return 1;
}

static void loadSymbolsImpl(DynamicLibrary& dll, std::vector<DebugSymbol>& vector)
{
	DWORD options = SymGetOptions();
	options &= ~SYMOPT_DEFERRED_LOADS;
	options |= SYMOPT_LOAD_LINES;
	options |= SYMOPT_IGNORE_NT_SYMPATH;
	options |= SYMOPT_DEBUG;
	options |= SYMOPT_UNDNAME;
	SymSetOptions(options);

	HANDLE hProcess = GetCurrentProcess();
	if(!SymInitialize(hProcess, nullptr, true)) {
		throw Exception("Unable to initialize Symbol loading");
	}

	long long baseAddr = long long(dll.getBaseAddress());

	SymEnumSymbols(hProcess, baseAddr, "*vftable*", loadSymbolsCallback, &vector);

	SymCleanup(hProcess);
}

#else

static void loadSymbolsImpl(DynamicLibrary&, std::vector<DebugSymbol>&)
{
}

#endif

constexpr size_t mask = 0x8000000000000001;

DebugSymbol::DebugSymbol(std::string name, void* address, size_t size)
	: name(name)
	, address(size_t(address) ^ mask)
	, size(size)
{
}

void* DebugSymbol::getAddress() const
{
	return reinterpret_cast<void*>(address ^ mask);
}

std::vector<DebugSymbol> SymbolLoader::loadSymbols(DynamicLibrary& dll)
{
	std::vector<DebugSymbol> results;
	loadSymbolsImpl(dll, results);
	return results;
}

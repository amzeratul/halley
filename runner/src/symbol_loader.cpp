#include "symbol_loader.h"
#include <halley/support/exception.h>
#include <iostream>
#include "dynamic_library.h"
#include <sstream>

using namespace Halley;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")

static BOOL CALLBACK loadSymbolsCallback(SYMBOL_INFO* symInfo, unsigned long symbolSize, void* userContext)
{
	auto& symbols = *reinterpret_cast<std::vector<DebugSymbol>*>(userContext);
	
	std::string name(symInfo->Name, symInfo->NameLen);
	if (name.find("table") != std::string::npos) {
		if (name.find("`vftable'") != std::string::npos || name.find("`vbtable'") != std::string::npos) {
			symbols.push_back(DebugSymbol(symInfo->Name, reinterpret_cast<void*>(symInfo->Address), sizeof(void*)));
		}
	}

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

	SymEnumSymbols(hProcess, baseAddr, "*", loadSymbolsCallback, &vector);

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

void DebugSymbol::appendToName(size_t id)
{
	std::stringstream ss;
	ss << "#" << id;
	name += ss.str();
}

std::vector<DebugSymbol> SymbolLoader::loadSymbols(DynamicLibrary& dll)
{
	std::vector<DebugSymbol> results;
	loadSymbolsImpl(dll, results);

	// We need to distinguish between multiple symbols with the same name.
	// This is typically the case when doing multiple inheritance, as all vftables will have the same name.

	// Our technique here is to sort the list of symbols by name, then by address, then rename conflicting types to have a number appended to them
	std::sort(results.begin(), results.end(), [](DebugSymbol& a, DebugSymbol& b) -> bool {
		if (a.getName() < b.getName()) return true;
		if (b.getName() < a.getName()) return false;
		return a.getAddress() < b.getAddress();
	});
	results.push_back(DebugSymbol("--sigil--", nullptr, 0));

	std::string curName;
	size_t lastUnique = 0;
	for (size_t i = 0; i < results.size(); i++) {
		if (results[i].getName() != curName) {
			// Go through all the repeated ones and rename them
			size_t nUnique = i - lastUnique;
			if (nUnique >= 2) {
				for (size_t j = 0; j < nUnique; j++) {
					results[j + lastUnique].appendToName(j);
				}
			}

			// New name
			lastUnique = i;
			curName = results[i].getName();
		}		
	}

	// Remove sigil
	results.pop_back();

	return results;
}

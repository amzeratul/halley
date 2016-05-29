#pragma once

#include <halley/text/halleystring.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
using LibHandleType = HMODULE;
#else
using LibHandleType = void*;
#endif

namespace Halley
{
	class DynamicLibrary
	{
	public:
		DynamicLibrary(String name);
		~DynamicLibrary();

		void load(bool withAnotherName);
		void unload();

		void* getFunction(String name);
		void* getBaseAddress() const;

		bool hasChanged() const;

	private:
		String libName;
		String libOrigPath;
		String libPath;
		String debugSymbolsPath;
		bool hasTempPath = false;
		bool hasDebugSymbols = false;

		bool loaded = false;
		LibHandleType handle;

		time_t libLastWrite;
		time_t debugLastWrite;
	};
}

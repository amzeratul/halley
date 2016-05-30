#pragma once

#include <boost/filesystem/path.hpp>
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
		DynamicLibrary(std::string name);
		~DynamicLibrary();

		void load(bool withAnotherName);
		void unload();

		void* getFunction(std::string name) const;
		void* getBaseAddress() const;

		bool hasChanged() const;

	private:
		std::string libName;
		boost::filesystem::path libOrigPath;
		boost::filesystem::path libPath;
		boost::filesystem::path debugSymbolsOrigPath;
		boost::filesystem::path debugSymbolsPath;

		LibHandleType handle = nullptr;

		time_t libLastWrite = 0;
		time_t debugLastWrite = 0;

		bool hasTempPath = false;
		bool hasDebugSymbols = false;
		bool loaded = false;

	};
}

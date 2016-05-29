#pragma once

#include <halley/text/halleystring.h>
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

		void* getFunction(std::string name);
		void* getBaseAddress() const;

		bool hasChanged() const;

	private:
		std::string libName;
		boost::filesystem::path libOrigPath;
		boost::filesystem::path libPath;
		boost::filesystem::path debugSymbolsOrigPath;
		boost::filesystem::path debugSymbolsPath;

		LibHandleType handle;

		time_t libLastWrite;
		time_t debugLastWrite;

		bool hasTempPath = false;
		bool hasDebugSymbols = false;
		bool loaded = false;

	};
}

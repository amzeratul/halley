#pragma once

#include <filesystem>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#ifdef min
#undef min
#undef max
#endif
using LibHandleType = HMODULE;
#else
using LibHandleType = void*;
#endif

namespace Halley
{
	class DynamicLibrary
	{
	public:
		DynamicLibrary(std::string originalPath);
		~DynamicLibrary();

		void load(bool withAnotherName);
		void unload();

		void* getFunction(std::string name) const;
		void* getBaseAddress() const;

		bool hasChanged() const;

	private:
		std::string libName;
		std::filesystem::path libOrigPath;
		std::filesystem::path libPath;
		std::filesystem::path debugSymbolsOrigPath;
		std::filesystem::path debugSymbolsPath;

		LibHandleType handle = nullptr;

		std::filesystem::file_time_type libLastWrite;
		std::filesystem::file_time_type debugLastWrite;

		bool hasTempPath = false;
		bool hasDebugSymbols = false;
		bool loaded = false;

		mutable std::vector<std::filesystem::path> toDelete;
		void flushLoaded() const;
	};
}

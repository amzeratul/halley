#pragma once

#include <filesystem>
using LibHandleType = void*;

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

		bool isLoaded() const;
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

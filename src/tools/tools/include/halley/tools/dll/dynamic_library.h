#pragma once

#include <boost/filesystem.hpp>

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

		mutable std::vector<boost::filesystem::path> toDelete;
		void flushLoaded() const;
	};
}

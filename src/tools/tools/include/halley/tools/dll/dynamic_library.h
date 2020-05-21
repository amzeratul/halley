#pragma once

#include <set>
#include <boost/filesystem.hpp>

using LibHandleType = void*;

namespace Halley
{
	class IDynamicLibraryListener {
	public:
		virtual ~IDynamicLibraryListener() = default;
		virtual void onUnloadDLL() = 0;
		virtual void onLoadDLL() = 0;
	};
	
	class DynamicLibrary
	{
	public:
		DynamicLibrary(std::string originalPath);
		~DynamicLibrary();

		bool load(bool withAnotherName);
		void unload();

		void* getFunction(std::string name) const;
		void* getBaseAddress() const;

		bool isLoaded() const;
		bool hasChanged() const;

		void reloadIfChanged();
		void notifyReload();
		void notifyUnload();
		void addReloadListener(IDynamicLibraryListener& listener);
		void removeReloadListener(IDynamicLibraryListener& listener);

		void clearTempDirectory();

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
		bool waitingReload = false;

		mutable std::vector<boost::filesystem::path> toDelete;

		std::set<IDynamicLibraryListener*> reloadListeners;

		void flushLoaded() const;
		boost::filesystem::path getTempPath() const;
	};
}

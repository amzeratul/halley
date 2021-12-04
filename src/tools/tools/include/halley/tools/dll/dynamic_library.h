#pragma once

#include <set>

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
		DynamicLibrary(std::string originalPath, bool includeDebugSymbols);
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
		std::string libOrigPath;
		std::string libPath;
		std::string debugSymbolsOrigPath;
		std::string debugSymbolsPath;

		LibHandleType handle = nullptr;

		time_t libLastWrite = 0;
		time_t debugLastWrite = 0;

		bool hasTempPath = false;
		bool hasDebugSymbols = false;
		bool loaded = false;
		bool waitingReload = false;
		bool includeDebugSymbols = false;

		mutable std::vector<std::string> toDelete;

		std::set<IDynamicLibraryListener*> reloadListeners;

		void flushLoaded() const;
		std::string getTempPath() const;
	};
}

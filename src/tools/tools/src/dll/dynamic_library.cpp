#include "halley/tools/dll/dynamic_library.h"

#include <halley/support/exception.h>
#include <filesystem>

#include "halley/concurrency/concurrent.h"
#include "halley/maths/random.h"
#include "halley/maths/uuid.h"
#include "halley/support/logger.h"
#include "halley/text/encode.h"
#include "halley/text/string_converter.h"
#include "halley/tools/runner/memory_patcher.h"
#include "halley/tools/runner/symbol_loader.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#endif

using namespace Halley;
using namespace std::filesystem;

DynamicLibrary::DynamicLibrary(std::string originalPath, bool includeDebugSymbols)
	: libOrigPath(originalPath)
	, includeDebugSymbols(includeDebugSymbols)
{
	libName = path(originalPath).filename().string();
}

DynamicLibrary::~DynamicLibrary()
{
	unload();
}

bool DynamicLibrary::load(bool withAnotherName)
{
	unload();

	// Does the path exist?
	if (std::error_code ec; !exists(libOrigPath, ec) || ec) {
		Logger::logError("Library doesn't exist: " + libOrigPath, true);
		return false;
	}

	// Determine which path to load
	hasTempPath = withAnotherName;
	if (withAnotherName) {
		auto tmpPath = getTempPath();
		create_directories(tmpPath);
		
		libPath = (std::filesystem::path(tmpPath) / String("halley-" + UUID::generate().toString() + ".dll").cppStr()).string();

		std::error_code ec;
		bool success = false;
		for (int i = 0; i < 3 && !success; ++i) {
			copy_file(libOrigPath, libPath, ec);
			if (ec) {
				using namespace std::chrono_literals;
				std::this_thread::sleep_for((i + 1) * 0.1s);
			} else {
				success = true;
			}
		}

		if (!success) {
			Logger::logError("Error copying DLL \"" + libOrigPath + "\": " + ec.message());
			return false;
		}
	} else {
		libPath = libOrigPath;
	}

	// Check for debug symbols
	if (includeDebugSymbols) {
		debugSymbolsOrigPath = libOrigPath;
		#ifdef _WIN32
		debugSymbolsOrigPath = std::filesystem::path(debugSymbolsOrigPath).replace_extension("pdb").string();
		#endif
		hasDebugSymbols = exists(debugSymbolsOrigPath);
	}

	// Copy debug symbols if the lib got copied
	/*
	if (withAnotherName && debugSymbolsOrigPath != libOrigPath) {
		debugSymbolsPath = libPath;
		#ifdef _WIN32
		debugSymbolsPath.replace_extension("pdb");
		#endif
		copy_file(debugSymbolsOrigPath, debugSymbolsPath);
	} else {
		debugSymbolsPath = debugSymbolsOrigPath;
	}
	*/

	// Load
	#ifdef _WIN32
	handle = LoadLibraryW(std::filesystem::path(libPath).wstring().c_str());
	#endif
	if (!handle) {
		Logger::logError("Unable to load library: " + libPath);
		unload();
		return false;
	}

	// Store write times
	for (size_t i = 0; i < 3; ++i) {
		std::error_code ec0;
		std::error_code ec1;
		libLastWrite = last_write_time(libOrigPath, ec0);
		if (hasDebugSymbols) {
			debugLastWrite = last_write_time(debugSymbolsOrigPath, ec1);
		}
		if (!ec0 && !ec1) {
			loaded = true;
			return true;
		}

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(200ms);
	}
	
	return false;
}

void DynamicLibrary::unload()
{
	// WARNING: Don't call any Halley globals here (especially Logger)
	// This is because this can be called while hot-reloading DLLs, where Halley globals are undefined
	
	if (loaded) {
		#ifdef _WIN32
		if (!FreeLibrary(static_cast<HMODULE>(handle))) {
			throw Exception("Unable to release library " + libPath + " due to error " + toString(GetLastError()), HalleyExceptions::Core);
		}
		#endif
		handle = nullptr;

		if (hasTempPath) {
			toDelete.push_back(libPath);
			/*
			if (libPath != debugSymbolsPath) {
				toDelete.push_back(debugSymbolsPath);
			}
			*/
			flushLoaded();
		}

		loaded = false;
	}
}

void* DynamicLibrary::getFunction(std::string name) const
{
	if (!loaded) {
		return nullptr;
	}
	
	#ifdef _WIN32
	return GetProcAddress(static_cast<HMODULE>(handle), name.c_str());
	#else
	// TODO
	return nullptr;
	#endif
}

void* DynamicLibrary::getBaseAddress() const
{
	Expects(loaded);
	return handle;
}

size_t DynamicLibrary::getImageSize() const
{
#ifdef _WIN32
	MODULEINFO info;
	if (GetModuleInformation(GetCurrentProcess(), static_cast<HMODULE>(handle), &info, sizeof(info))) {
		return info.SizeOfImage;
	} else {
		return 0;
	}
#else
	return 0;
#endif
}

bool DynamicLibrary::isLoaded() const
{
	return loaded;
}

bool DynamicLibrary::hasChanged() const
{
	if (!loaded) {
		return false;
	}

	flushLoaded();
	
	// Never got debug symbols, so disable hot-reload
	if (includeDebugSymbols && !hasDebugSymbols) {
		return false;
	}
	// One of the files is missing, maybe there was a linker error
	if (!exists(libOrigPath) || (includeDebugSymbols && !exists(debugSymbolsOrigPath))) {
		return false;
	}

	// If BOTH the dll and debug symbols files have changed, we're ready to reload
	std::error_code ec;
	const auto libWrite = last_write_time(libOrigPath, ec);
	if (ec) {
		return false;
	}
	if (libWrite <= libLastWrite) {
		return false;
	}

	if (includeDebugSymbols) {
		const auto debugWrite = last_write_time(debugSymbolsOrigPath, ec);
		if (ec) {
			return false;
		}
		if (debugWrite <= debugLastWrite) {
			return false;
		}
	}

	return true;
}

void DynamicLibrary::reloadIfChanged(bool forceReload)
{
	constexpr bool checkMemory = false;

	if (forceReload || hasChanged()) {
		MemoryPatchingMappings mappings;
		if (checkMemory) {
			Logger::logDev("DLL in memory range 0x" + toString(size_t(handle), 16, 16).asciiUpper() + " to 0x" + toString(size_t(handle) + getImageSize(), 16, 16).asciiUpper());
			mappings.generate(SymbolLoader::loadSymbols(*this));
		}
		
		notifyUnload();
		unload();

		if (checkMemory) {
			MemoryPatcher::patch(mappings);
		}

		waitingReload = true;
	}

	if (waitingReload) {
		if (load(true)) {
			notifyReload();
			waitingReload = false;
		}
	}
}

void DynamicLibrary::notifyReload()
{
	for (const auto& l: reloadListeners) {
		l->onLoadDLL();
	}
}

void DynamicLibrary::notifyUnload()
{
	for (const auto& l: reloadListeners) {
		l->onUnloadDLL();
	}
}

void DynamicLibrary::addReloadListener(IDynamicLibraryListener& listener)
{
	reloadListeners.insert(&listener);
}

void DynamicLibrary::removeReloadListener(IDynamicLibraryListener& listener)
{
	reloadListeners.erase(&listener);
}

void DynamicLibrary::clearTempDirectory()
{
	std::error_code ec;
	std::filesystem::remove_all(getTempPath(), ec);
}

void DynamicLibrary::flushLoaded() const
{
	// WARNING: Don't call any Halley globals here (especially Logger)
	decltype(toDelete) remaining;
	
	for (auto& f: toDelete) {
		std::error_code ec;
		if (!std::filesystem::remove(f, ec)) {
			remaining.push_back(std::move(f));
		}
	}

	toDelete = std::move(remaining);
}

std::string DynamicLibrary::getTempPath() const
{
	return (std::filesystem::path(libOrigPath).parent_path() / "halley_tmp").string();
}

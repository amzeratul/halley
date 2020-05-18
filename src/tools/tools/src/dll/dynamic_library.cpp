#include "halley/tools/dll/dynamic_library.h"
#include <halley/support/exception.h>

#include "halley/maths/random.h"
#include "halley/support/logger.h"
#include "halley/text/encode.h"
#include "halley/text/string_converter.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#ifdef min
#undef min
#undef max
#endif
#endif

using namespace Halley;
using namespace boost::filesystem;

DynamicLibrary::DynamicLibrary(std::string originalPath)
	: libOrigPath(originalPath)
{
	libName = path(originalPath).filename().string();
}

DynamicLibrary::~DynamicLibrary()
{
	unload();
}

void DynamicLibrary::load(bool withAnotherName)
{
	unload();

	// Determine which path to load
	hasTempPath = withAnotherName;
	if (withAnotherName) {
		auto tmpPath = getTempPath();
		create_directories(tmpPath);

		Bytes randomBytes(8);
		Random::getGlobal().getBytes(randomBytes);
		libPath = tmpPath / String("halley-" + Encode::encodeBase16(randomBytes) + ".dll").cppStr();

		bool success = false;
		for (int i = 0; i < 3 && !success; ++i) {
			boost::system::error_code ec;
			copy_file(libOrigPath, libPath, ec);
			if (ec.failed()) {
				using namespace std::chrono_literals;
				std::this_thread::sleep_for((i + 1) * 1s);
			} else {
				success = true;
			}
		}

		if (!success) {
			throw Exception("Source DLL is locked.", HalleyExceptions::Tools);
		}
	} else {
		libPath = libOrigPath;
	}

	// Check for debug symbols
	debugSymbolsOrigPath = libOrigPath;
	#ifdef _WIN32
	debugSymbolsOrigPath.replace_extension("pdb");
	#endif
	hasDebugSymbols = exists(debugSymbolsOrigPath);

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
	handle = LoadLibraryW(libPath.wstring().c_str());
	#endif
	if (!handle) {
		throw Exception("Unable to load library: " + libPath.string(), HalleyExceptions::Core);
	}

	// Store write times
	if (hasDebugSymbols) {
		libLastWrite = last_write_time(libOrigPath);
		debugLastWrite = last_write_time(debugSymbolsOrigPath);
	}

	loaded = true;
}

void DynamicLibrary::unload()
{
	// WARNING: Don't call any Halley globals here (especially Logger)
	// This is because this can be called while hot-reloading DLLs, where Halley globals are undefined
	
	if (loaded) {
		#ifdef _WIN32
		if (!FreeLibrary(static_cast<HMODULE>(handle))) {
			throw Exception("Unable to release library " + libPath.string() + " due to error " + toString(GetLastError()), HalleyExceptions::Core);
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
	Expects(loaded);
	
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
	if (!hasDebugSymbols) {
		return false;
	}
	// One of the files is missing, maybe there was a linker error
	if (!exists(libOrigPath) || !exists(debugSymbolsOrigPath)) {
		return false;
	}

	// If BOTH the dll and debug symbols files have changed, we're ready to reload
	boost::system::error_code ec;
	const auto libWrite = last_write_time(libOrigPath, ec);
	if (ec.failed()) {
		return false;
	}
	const auto debugWrite = last_write_time(debugSymbolsOrigPath, ec);
	if (ec.failed()) {
		return false;
	}
	return libWrite > libLastWrite && debugWrite > debugLastWrite;
}

void DynamicLibrary::reloadIfChanged()
{
	if (hasChanged()) {
		for (const auto& l: reloadListeners) {
			l->onUnloadDLL();
		}
		
		unload();
		load(true);
		
		for (const auto& l: reloadListeners) {
			l->onLoadDLL();
		}
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
	boost::system::error_code ec;
	boost::filesystem::remove_all(getTempPath(), ec);
}

void DynamicLibrary::flushLoaded() const
{
	// WARNING: Don't call any Halley globals here (especially Logger)
	decltype(toDelete) remaining;
	
	for (auto& f: toDelete) {
		boost::system::error_code ec;
		if (!boost::filesystem::remove(f, ec)) {
			remaining.push_back(std::move(f));
		}
	}

	toDelete = std::move(remaining);
}

path DynamicLibrary::getTempPath() const
{
	return libOrigPath.parent_path() / "halley_tmp";
}

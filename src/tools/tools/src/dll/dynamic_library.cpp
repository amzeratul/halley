#include "halley/tools/dll/dynamic_library.h"
#include <filesystem>
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
using namespace std::filesystem;

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
		auto tmpPath = libOrigPath.parent_path() / "halley_tmp";
		create_directories(tmpPath);

		Bytes randomBytes(8);
		Random::getGlobal().getBytes(randomBytes);
		libPath = tmpPath / String("halley-" + Encode::encodeBase16(randomBytes) + ".dll").cppStr();
		copy_file(libOrigPath, libPath);
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

bool DynamicLibrary::hasChanged() const
{
	Expects(loaded);

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
	return last_write_time(libOrigPath) > libLastWrite && last_write_time(debugSymbolsOrigPath) > debugLastWrite;
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

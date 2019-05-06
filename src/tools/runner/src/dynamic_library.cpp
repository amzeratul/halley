#include "dynamic_library.h"
#include <boost/filesystem.hpp>
#include <halley/support/exception.h>
#include "halley/text/string_converter.h"

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
		libPath = libOrigPath.parent_path() / unique_path("halley-%%%%-%%%%-%%%%-%%%%.dll");
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
	handle = LoadLibrary(libPath.string().c_str());
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
	if (loaded) {
		#ifdef _WIN32
		if (!FreeLibrary(handle)) {
			throw Exception("Unable to release library " + libPath.string() + " due to error " + toString(GetLastError()), HalleyExceptions::Core);
		}
		#endif
		handle = nullptr;

		if (hasTempPath) {
			remove(libPath);
			/*
			if (libPath != debugSymbolsPath) {
				remove(debugSymbolsPath);
			}
			*/
		}

		loaded = false;
	}
}

void* DynamicLibrary::getFunction(std::string name) const
{
	#ifdef _WIN32
	return reinterpret_cast<void*>(GetProcAddress(handle, name.c_str()));
	#else
	return nullptr;
	#endif
}

void* DynamicLibrary::getBaseAddress() const
{
	return handle;
}

bool DynamicLibrary::hasChanged() const
{
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

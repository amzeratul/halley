#include "dynamic_library.h"
#include <boost/filesystem.hpp>
#include <halley/support/exception.h>
#include <iostream>

using namespace Halley;
using namespace boost::filesystem;

DynamicLibrary::DynamicLibrary(String name)
	: libName(name)
{
	#ifdef _WIN32
	libOrigPath = path((libName + ".dll").cppStr());
	#endif
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
		libPath = unique_path("halley-%%%%-%%%%-%%%%-%%%%.dll").string();
		std::cout << "Copying " << libOrigPath.string() << " to " << libPath.string() << std::endl;
		copy_file(libOrigPath, libPath);
	} else {
		libPath = libOrigPath;
	}

	// Check for debug symbols
	debugSymbolsPath = libOrigPath;
	#ifdef _WIN32
	debugSymbolsPath.replace_extension("pdb");
	#endif
	hasDebugSymbols = exists(debugSymbolsPath);

	// Load
	#ifdef _WIN32
	handle = LoadLibrary(libPath.string().c_str());
	#endif
	if (!handle) {
		throw Exception("Unable to load library: " + libPath.string());
	}

	// Store write times
	if (hasDebugSymbols) {
		libLastWrite = last_write_time(libOrigPath);
		debugLastWrite = last_write_time(debugSymbolsPath);
	}

	loaded = true;
}

void DynamicLibrary::unload()
{
	if (loaded) {
		#ifdef _WIN32
		FreeLibrary(handle);
		#endif
		handle = nullptr;

		if (hasTempPath) {
			remove(libPath);
		}

		loaded = false;
	}
}

void* DynamicLibrary::getFunction(String name)
{
	#ifdef _WIN32
	return GetProcAddress(handle, name.c_str());
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
	if (!exists(libOrigPath) || !exists(debugSymbolsPath)) {
		return false;
	}
	// If BOTH the dll and debug symbols files have changed, we're ready to reload
	return last_write_time(libOrigPath) > libLastWrite && last_write_time(debugSymbolsPath) > debugLastWrite;
}

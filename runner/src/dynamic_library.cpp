#include "dynamic_library.h"
#include <boost/filesystem.hpp>
#include <halley/support/exception.h>

using namespace Halley;
using namespace boost::filesystem;

DynamicLibrary::DynamicLibrary(String name)
	: libName(name)
{
	#ifdef _WIN32
	libOrigPath = libName + ".dll";
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
		copy_file(path(libOrigPath.cppStr()), libPath.cppStr());
	} else {
		libPath = libOrigPath;
	}

	// Check for debug symbols
	debugSymbolsPath = libOrigPath;
	#ifdef _WIN32
	auto debugPath = path(debugSymbolsPath.cppStr()).replace_extension("pdb");
	hasDebugSymbols = exists(debugPath);
	debugSymbolsPath = debugPath.string();
	#endif

	// Load
	#ifdef _WIN32
	handle = LoadLibrary(libPath.c_str());
	#endif
	if (!handle) {
		throw Exception("Unable to load library: " + libPath);
	}

	// Store write times
	if (hasDebugSymbols) {
		libLastWrite = last_write_time(libOrigPath.cppStr());
		debugLastWrite = last_write_time(debugSymbolsPath.cppStr());
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
			remove(path(libPath.cppStr()));
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
	if (!hasDebugSymbols) {
		return false;
	}
	return last_write_time(libOrigPath.cppStr()) > libLastWrite && last_write_time(debugSymbolsPath.cppStr()) > debugLastWrite;
}

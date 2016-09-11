/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#include <boost/filesystem.hpp>
#include <halley/support/exception.h>
#include <cassert>
#include <iostream>
#include <halley/support/console.h>
#include "resources/resource_filesystem.h"
#include "halley/core/api/system_api.h"

using namespace Halley;

FileSystemResourceLocator::FileSystemResourceLocator(SystemAPI& system, String _basePath)
	: system(system)
    , basePath(_basePath)
{
}

std::unique_ptr<ResourceData> FileSystemResourceLocator::doGet(String resource, bool stream)
{
	String path = basePath + resource;
	
	if (stream) {
		return std::make_unique<ResourceDataStream>(path, [=] () -> std::unique_ptr<ResourceDataReader> {
			return system.getDataReader(path.cppStr());
		});
	} else {
		auto fp = system.getDataReader(path.cppStr());
		if (!fp) {
			return std::unique_ptr<ResourceDataStatic>();
		}

		size_t size = fp->size();
		char* buf = new char[size];
		fp->read(gsl::as_writeable_bytes(gsl::span<char>(buf, size)));
		return std::make_unique<ResourceDataStatic>(buf, size, path);
	}
}

std::time_t Halley::FileSystemResourceLocator::doGetTimestamp(String resource)
{
	namespace fs = boost::filesystem;
	String pathName = basePath + resource;
	fs::path path(pathName.c_str());
	if (fs::exists(path)) {
		return fs::last_write_time(path);
		//auto time = fs::last_write_time(path);
		//return decltype(time)::clock::to_time_t(time);
	} else {
		return 0;
	}
}

static void enumDir(StringArray& files, String root, String prefix)
{
	namespace fs = boost::filesystem;
	String basePath = (root + prefix + "/");
	basePath.replace("//", "/");
	basePath.replace("\\\\", "\\");
	fs::path rootPath = fs::path(basePath.c_str()).make_preferred();
	try {
		for (fs::directory_iterator end, dir(rootPath.string().c_str()); dir != end; ++dir) {
			fs::path curPath = (*dir).path();
			String name = curPath.filename().generic_string();
			fs::file_type type = (*dir).status().type();
			if (type == fs::file_type::directory_file) {
				if (!name.startsWith(".")) {
					enumDir(files, root, prefix + name + "/");
				}
			}
			else {
				files.push_back(prefix + name);
			}
		}
	} catch (std::exception &) {
		std::cout << ConsoleColour(Console::YELLOW) << "Unable to enumerate resources folder: " << rootPath << ConsoleColour() << std::endl;
	}
}
	
Vector<String> FileSystemResourceLocator::getResourceList()
{
	Vector<String> res;
	res.push_back("*");

#ifndef __ANDROID__
	enumDir(res, basePath, "assets/");
#endif

	return res;
}
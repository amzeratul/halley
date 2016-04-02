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

#include "resource_filesystem.h"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <SDL.h>
#include "resource_data_reader.h"

using namespace Halley;

FileSystemResourceLocator::FileSystemResourceLocator(String _basePath)
	: basePath(_basePath)
{
}

std::unique_ptr<ResourceData> FileSystemResourceLocator::doGet(String resource, bool stream)
{
	String path = basePath+resource;
	
	if (stream) {
		return std::make_unique<ResourceDataStream>(path, [=] () -> std::unique_ptr<ResourceDataReader> {
			SDL_RWops* fp = SDL_RWFromFile(path.c_str(), "rb");
			if (!fp) {
				throw Exception("Unable to open resource (FileSystem/Stream): "+resource);
			}

			SDL_RWseek(fp, 0, SEEK_END);
			Sint64 sz = SDL_RWtell(fp);
			SDL_RWseek(fp, 0, SEEK_SET);
			return std::make_unique<ResourceDataReaderFile>(fp, 0, static_cast<int>(sz), true);
		});
	}

	else {
		SDL_RWops* fp = SDL_RWFromFile(path.c_str(), "rb");
		if (!fp) {
			throw Exception("Unable to open resource (FileSystem/Static): "+resource);
		}

		SDL_RWseek(fp, 0, RW_SEEK_END);
		Sint64 p1 = SDL_RWtell(fp);
		SDL_RWseek(fp, 0, RW_SEEK_SET);
		if (p1 <= 0) {
			throw Exception("Invalid file size for resource (FileSystem/Static): " + resource + ", " + String::integerToString(int(p1)) + " bytes.");
		}
		size_t sz = size_t(p1);

		char* buf = new char[static_cast<unsigned int>(sz)];
		Sint64 nRead = SDL_RWread(fp, buf, 1, size_t(sz));
		assert (nRead == sz);
		(void) nRead;
		std::unique_ptr<ResourceData> data(new ResourceDataStatic(buf, size_t(sz), path));

		SDL_RWclose(fp);

		return data;
	}
}

std::time_t Halley::FileSystemResourceLocator::doGetTimestamp(String resource)
{
	namespace fs = boost::filesystem;
	String pathName = basePath + resource;
	fs::path path(pathName.c_str());
	if (fs::exists(path)) {
		return fs::last_write_time(path);
	} else {
		return 0;
	}
}

static void enumDir(StringArray& files, String root, String prefix)
{
	namespace fs = boost::filesystem;
	fs::path rootPath = fs::path((root + prefix + "/").c_str());
	for (fs::directory_iterator end, dir(rootPath.string().c_str()); dir != end; ++dir) {
		fs::path curPath = (*dir).path();
		String name = curPath.filename().generic_string();
		fs::file_type type = (*dir).status().type();
		if (type == fs::directory_file) {
			if (!name.startsWith(".")) {
				enumDir(files, root, prefix + name + "/");
			}
		} else {
			files.push_back(prefix + name);
		}		
	}
}
	
std::vector<String> FileSystemResourceLocator::getResourceList()
{
	std::vector<String> res;
	res.push_back("*");

#ifndef __ANDROID__
	enumDir(res, basePath, "data/");
#endif

	return res;
}
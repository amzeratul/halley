#pragma once

#include "prec.h"
#include <boost/filesystem.hpp>

namespace Halley
{
	using Path = boost::filesystem::path;

	class Project
	{
	public:
		Project(Path rootPath, Path sharedAssetsSrcPath);
		
		Path getAssetsPath() const;
		Path getAssetsSrcPath() const;
		Path getSharedAssetsSrcPath() const;
	
	private:
		Path rootPath;
		Path sharedAssetsSrcPath;
	};
}

#pragma once

#include "prec.h"
#include <boost/filesystem.hpp>

namespace Halley
{
	class Project
	{
	public:
		Project(boost::filesystem::path rootPath, boost::filesystem::path sharedAssetsPath);

	private:
		boost::filesystem::path rootPath;
		boost::filesystem::path sharedAssetsPath;
	};
}

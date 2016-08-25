#include "project.h"

using namespace Halley;

Project::Project(boost::filesystem::path rootPath, boost::filesystem::path sharedAssetsPath)
	: rootPath(rootPath)
	, sharedAssetsPath(sharedAssetsPath)
{}

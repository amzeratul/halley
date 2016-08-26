#include "project.h"

using namespace Halley;

Project::Project(Path rootPath, Path sharedAssetsSrcPath)
	: rootPath(rootPath)
	, sharedAssetsSrcPath(sharedAssetsSrcPath)
{}

Path Project::getAssetsPath() const
{
	return rootPath / "assets";
}

Path Project::getAssetsSrcPath() const
{
	return rootPath / "assets_src";
}

Path Project::getSharedAssetsSrcPath() const
{
	return sharedAssetsSrcPath;
}

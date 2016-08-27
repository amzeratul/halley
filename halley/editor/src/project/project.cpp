#include "project.h"
#include "src/tasks/assets/import_assets_database.h"

using namespace Halley;

Project::Project(Path rootPath, Path sharedAssetsSrcPath)
	: rootPath(rootPath)
	, sharedAssetsSrcPath(sharedAssetsSrcPath)
{
	importAssetsDatabase = std::make_unique<ImportAssetsDatabase>(rootPath / "assets" / "import.db");
}

Project::~Project() = default;

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

ImportAssetsDatabase& Project::getImportAssetsDatabase()
{
	return *importAssetsDatabase;
}

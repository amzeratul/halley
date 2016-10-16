#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/project/project.h"

using namespace Halley;

Project::Project(Path rootPath, Path sharedAssetsSrcPath)
	: rootPath(rootPath)
	, sharedAssetsSrcPath(sharedAssetsSrcPath)
{
	importAssetsDatabase = std::make_unique<ImportAssetsDatabase>(getAssetsPath(), getAssetsPath() / "import.db");
	codegenDatabase = std::make_unique<ImportAssetsDatabase>(getGenPath(), getGenPath() / "import.db");
	assetImporter = std::make_unique<AssetImporter>(std::vector<Path>{getSharedAssetsSrcPath(), getAssetsSrcPath()});
}

Project::~Project() = default;

Path Project::getAssetsPath() const
{
	return rootPath / "bin" / "assets";
}

Path Project::getAssetsSrcPath() const
{
	return rootPath / "assets_src";
}

Path Project::getSharedAssetsSrcPath() const
{
	return sharedAssetsSrcPath;
}

Path Project::getGenPath() const
{
	return rootPath / "gen";
}

Path Project::getGenSrcPath() const
{
	return rootPath / "gen_src";
}

ImportAssetsDatabase& Project::getImportAssetsDatabase()
{
	return *importAssetsDatabase;
}

ImportAssetsDatabase& Project::getCodegenDatabase()
{
	return *codegenDatabase;
}

const AssetImporter& Project::getAssetImporter() const
{
	return *assetImporter;
}

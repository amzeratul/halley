#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/project/project.h"
#include "halley/tools/file/filesystem.h"

using namespace Halley;

Project::Project(const String& platform, Path projectRootPath, Path halleyRootPath)
	: platform(platform)
	, rootPath(projectRootPath)
	, halleyRootPath(halleyRootPath)
{
	importAssetsDatabase = std::make_unique<ImportAssetsDatabase>(getAssetsPath(), getAssetsPath() / "import.db");
	codegenDatabase = std::make_unique<ImportAssetsDatabase>(getGenPath(), getGenPath() / "import.db");

	initialisePlugins();
	assetImporter = std::make_unique<AssetImporter>(*this, std::vector<Path>{getSharedAssetsSrcPath(), getAssetsSrcPath()});
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
	return halleyRootPath / "shared_assets";
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

std::unique_ptr<IAssetImporter> Project::getAssetImporterOverride(AssetType type) const
{
	// TODO
	return {};
}

void Project::initialisePlugins()
{
	auto pluginPath = halleyRootPath / "plugins";
	auto files = FileSystem::enumerateDirectory(pluginPath);
	for (auto& file: files) {
		if (file.getExtension() == ".dll") {
			loadPlugin(file);
		}
	}
}

void Project::loadPlugin(const Path& path)
{
	
}

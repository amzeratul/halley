#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/project/project.h"
#include "halley/tools/file/filesystem.h"
#include "halley/core/game/halley_statics.h"

using namespace Halley;

Project::Project(const String& platform, Path projectRootPath, Path halleyRootPath, std::vector<HalleyPluginPtr> plugins)
	: platform(platform)
	, rootPath(projectRootPath)
	, halleyRootPath(halleyRootPath)
	, plugins(std::move(plugins))
{
	importAssetsDatabase = std::make_unique<ImportAssetsDatabase>(getUnpackedAssetsPath(), getUnpackedAssetsPath() / "import.db", getUnpackedAssetsPath() / "assets.db", platform);
	codegenDatabase = std::make_unique<ImportAssetsDatabase>(getGenPath(), getGenPath() / "import.db", getGenPath() / "assets.db", "");
	assetImporter = std::make_unique<AssetImporter>(*this, std::vector<Path>{getSharedAssetsSrcPath(), getAssetsSrcPath()});
}

Project::~Project()
{
	assetImporter.reset();
	plugins.clear();
}

Path Project::getRootPath() const
{
	return rootPath;
}

Path Project::getUnpackedAssetsPath() const
{
	return rootPath / "assets_unpacked";
}

Path Project::getPackedAssetsPath() const
{
	return rootPath / "assets";
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

Path Project::getAssetPackManifestPath() const
{
	return assetPackManifest;
}

ImportAssetsDatabase& Project::getImportAssetsDatabase() const
{
	return *importAssetsDatabase;
}

ImportAssetsDatabase& Project::getCodegenDatabase() const
{
	return *codegenDatabase;
}

const AssetImporter& Project::getAssetImporter() const
{
	return *assetImporter;
}

std::unique_ptr<IAssetImporter> Project::getAssetImporterOverride(ImportAssetType type) const
{
	for (auto& plugin: plugins) {
		auto importer = plugin->getAssetImporter(type);
		if (importer) {
			return importer;
		}
	}
	return {};
}

void Project::setAssetPackManifest(const Path& path)
{
	assetPackManifest = path;
}

void Project::setDevConServer(DevConServer* server)
{
	devConServer = server;
}

DevConServer* Project::getDevConServer() const
{
	return devConServer;
}

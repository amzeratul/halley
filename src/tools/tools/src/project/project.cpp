#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/project/project.h"
#include "halley/tools/file/filesystem.h"
#include "halley/core/game/halley_statics.h"
#include "halley/tools/project/project_properties.h"
#include "halley/support/logger.h"
#include "halley/core/devcon/devcon_server.h"

using namespace Halley;

Project::Project(std::vector<String> _platforms, Path projectRootPath, Path halleyRootPath, std::vector<HalleyPluginPtr> plugins)
	: platforms(std::move(_platforms))
	, rootPath(projectRootPath)
	, halleyRootPath(halleyRootPath)
	, plugins(std::move(plugins))
{
	importAssetsDatabase = std::make_unique<ImportAssetsDatabase>(getUnpackedAssetsPath(), getUnpackedAssetsPath() / "import.db", getUnpackedAssetsPath() / "assets.db", platforms);
	codegenDatabase = std::make_unique<ImportAssetsDatabase>(getGenPath(), getGenPath() / "import.db", getGenPath() / "assets.db", std::vector<String>{ "" });
	assetImporter = std::make_unique<AssetImporter>(*this, std::vector<Path>{getSharedAssetsSrcPath(), getAssetsSrcPath()});
	properties = std::make_unique<ProjectProperties>(rootPath / "halley_project" / "properties.yaml");
}

Project::~Project()
{
	assetImporter.reset();
	plugins.clear();
}

std::vector<String> Project::getPlatforms() const
{
	return platforms;
}

Path Project::getRootPath() const
{
	return rootPath;
}

Path Project::getUnpackedAssetsPath() const
{
	return rootPath / "assets_unpacked";
}

Path Project::getPackedAssetsPath(const String& platform) const
{
	String suffix = platform == "pc" ? "" : "-" + platform;
	return rootPath / ("assets" + suffix);
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

std::vector<std::unique_ptr<IAssetImporter>> Project::getAssetImportersFromPlugins(ImportAssetType type) const
{
	std::vector<std::unique_ptr<IAssetImporter>> result;
	for (auto& plugin: plugins) {
		auto importer = plugin->getAssetImporter(type);
		if (importer) {
			result.emplace_back(std::move(importer));
		}
	}
	return result;
}

void Project::setAssetPackManifest(const Path& path)
{
	assetPackManifest = path;
}

void Project::setDevConServer(DevConServer* server)
{
	addAssetReloadCallback([=] (const std::vector<String>& assetIds) {
		server->reloadAssets(assetIds);
	});
}

void Project::addAssetReloadCallback(AssetReloadCallback callback)
{
	assetReloadCallbacks.push_back(std::move(callback));
}

ProjectProperties& Project::getProperties() const
{
	return *properties;
}

void Project::reloadAssets(const std::set<String>& assets)
{
	if (assetReloadCallbacks.empty()) {
		return;
	}

	std::vector<String> assetIds;
	assetIds.reserve(assets.size());
	for (auto& a: assets) {
		assetIds.push_back(a);
	}
	Logger::logInfo("Requesting reloading of " + toString(assetIds.size()) + " assets");

	for (auto& callback: assetReloadCallbacks) {
		callback(assetIds);
	}
}

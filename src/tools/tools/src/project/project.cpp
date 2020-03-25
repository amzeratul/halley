#include <utility>
#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/project/project.h"

#include "halley/core/api/halley_api.h"
#include "halley/tools/file/filesystem.h"
#include "halley/core/game/halley_statics.h"
#include "halley/tools/project/project_properties.h"
#include "halley/support/logger.h"
#include "halley/core/devcon/devcon_server.h"
#include "halley/core/resources/resource_locator.h"
#include "halley/core/resources/standard_resources.h"
#include "halley/tools/assets/metadata_importer.h"
#include "halley/tools/project/project_loader.h"

using namespace Halley;

Project::Project(Path projectRootPath, Path halleyRootPath, const ProjectLoader& loader)
	: rootPath(std::move(projectRootPath))
	, halleyRootPath(std::move(halleyRootPath))
{	
	properties = std::make_unique<ProjectProperties>(rootPath / "halley_project" / "properties.yaml");
	assetPackManifest = rootPath / properties->getAssetPackManifest();

	platforms = properties->getPlatforms();
	plugins = loader.getPlugins(platforms);

	importAssetsDatabase = std::make_unique<ImportAssetsDatabase>(getUnpackedAssetsPath(), getUnpackedAssetsPath() / "import.db", getUnpackedAssetsPath() / "assets.db", platforms);
	codegenDatabase = std::make_unique<ImportAssetsDatabase>(getGenPath(), getGenPath() / "import.db", getGenPath() / "assets.db", std::vector<String>{ "" });
	sharedCodegenDatabase = std::make_unique<ImportAssetsDatabase>(getSharedGenPath(), getSharedGenPath() / "import.db", getSharedGenPath() / "assets.db", std::vector<String>{ "" });
	assetImporter = std::make_unique<AssetImporter>(*this, std::vector<Path>{getSharedAssetsSrcPath(), getAssetsSrcPath()});

	auto dllPath = loader.getDLLPath(rootPath, properties->getDLL());
	if (!dllPath.isEmpty()) {
		gameDll = std::make_shared<DynamicLibrary>(dllPath.string());
		gameDll->load(false);
		Logger::logInfo("Loaded " + dllPath.string());
	}
}

Project::~Project()
{
	gameDll.reset();
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

Path Project::getSharedGenPath() const
{
	return halleyRootPath / "shared_gen";
}

Path Project::getSharedGenSrcPath() const
{
	return halleyRootPath / "shared_gen_src";
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

ImportAssetsDatabase& Project::getSharedCodegenDatabase() const
{
	return *sharedCodegenDatabase;
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
	addAssetPackReloadCallback([=] (const std::vector<String>& assetIds) {
		server->reloadAssets(assetIds);
	});
}

void Project::addAssetReloadCallback(AssetReloadCallback callback)
{
	assetReloadCallbacks.push_back(std::move(callback));
}

void Project::addAssetPackReloadCallback(AssetReloadCallback callback)
{
	assetPackedReloadCallbacks.push_back(std::move(callback));
}

ProjectProperties& Project::getProperties() const
{
	return *properties;
}

Metadata Project::getImportMetadata(AssetType type, const String& assetId) const
{
	return importAssetsDatabase->getMetadata(type, assetId).value_or(Metadata());
}

Metadata Project::readMetadataFromDisk(const Path& filePath) const
{
	Metadata metadata;
	const Path metaPath = getAssetsSrcPath() / filePath.replaceExtension(filePath.getExtension() + ".meta");
	MetadataImporter::loadMetaData(metadata, metaPath, false, filePath);
	return metadata;
}

void Project::writeMetadataToDisk(const Path& filePath, const Metadata& metadata)
{
	const auto str = metadata.toYAMLString();
	auto data = Bytes(str.size());
	memcpy(data.data(), str.c_str(), str.size());

	const Path metaPath = getAssetsSrcPath() / filePath.replaceExtension(filePath.getExtension() + ".meta");
	FileSystem::writeFile(metaPath, data);
	notifyAssetFileModified(filePath);
}

std::vector<String> Project::getAssetSrcList() const
{
	return importAssetsDatabase->getInputFiles();
}

std::vector<std::pair<AssetType, String>> Project::getAssetsFromFile(const Path& path) const
{
	return importAssetsDatabase->getAssetsFromFile(path);
}

void Project::reloadAssets(const std::set<String>& assets, bool packed)
{
	if (assetReloadCallbacks.empty()) {
		return;
	}

	// Build name list
	std::vector<String> assetIds;
	assetIds.reserve(assets.size());
	for (auto& a: assets) {
		assetIds.push_back(a);
	}

	// Reload game assets
	if (!packed && gameResources) {
		if (gameResources->getLocator().getLocatorCount() == 0) {
			try {
				gameResources->getLocator().addFileSystem(getUnpackedAssetsPath());
			} catch (...) {}
		}

		gameResources->reloadAssets(assetIds);
	}

	// Notify callbacks
	for (auto& callback: (packed ? assetPackedReloadCallbacks : assetReloadCallbacks)) {
		callback(assetIds);
	}
}

void Project::setCheckAssetTask(CheckAssetsTask* task)
{
	Expects(!checkAssetsTask ^ !task);
	checkAssetsTask = task;
}

void Project::notifyAssetFileModified(Path path)
{
	if (checkAssetsTask) {
		checkAssetsTask->requestRefreshAsset(std::move(path));
	}
}

const std::shared_ptr<DynamicLibrary>& Project::getGameDLL() const
{
	return gameDll;
}

void Project::loadGameResources(const HalleyAPI& api)
{
	auto locator = std::make_unique<ResourceLocator>(*api.system);
	try {
		locator->addFileSystem(getUnpackedAssetsPath());
	} catch (...) {}

	gameResources = std::make_unique<Resources>(std::move(locator), api);
	StandardResources::initialize(*gameResources);
}

Resources& Project::getGameResources()
{
	return *gameResources;
}

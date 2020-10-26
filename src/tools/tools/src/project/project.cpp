#include <utility>
#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/project/project.h"

#include "halley/core/api/halley_api.h"
#include "halley/tools/file/filesystem.h"
#include "halley/core/game/halley_statics.h"
#include "halley/tools/project/project_properties.h"
#include "halley/support/logger.h"
#include "halley/core/devcon/devcon_server.h"
#include "halley/core/entry/entry_point.h"
#include "halley/core/resources/resource_locator.h"
#include "halley/core/resources/standard_resources.h"
#include "halley/support/debug.h"
#include "halley/tools/assets/metadata_importer.h"
#include "halley/tools/ecs/ecs_data.h"
#include "halley/tools/project/project_loader.h"
#include "halley/core/game/game.h"
#include "halley/tools/yaml/yaml_convert.h"

using namespace Halley;

Project::Project(Path projectRootPath, Path halleyRootPath)
	: rootPath(std::move(projectRootPath))
	, halleyRootPath(std::move(halleyRootPath))
{	
	properties = std::make_unique<ProjectProperties>(rootPath / "halley_project" / "properties.yaml");
	assetPackManifest = rootPath / properties->getAssetPackManifest();

	platforms = properties->getPlatforms();

	importAssetsDatabase = std::make_unique<ImportAssetsDatabase>(getUnpackedAssetsPath(), getUnpackedAssetsPath() / "import.db", getUnpackedAssetsPath() / "assets.db", platforms);
	codegenDatabase = std::make_unique<ImportAssetsDatabase>(getGenPath(), getGenPath() / "import.db", getGenPath() / "assets.db", std::vector<String>{ "" });
	sharedCodegenDatabase = std::make_unique<ImportAssetsDatabase>(getSharedGenPath(), getSharedGenPath() / "import.db", getSharedGenPath() / "assets.db", std::vector<String>{ "" });

	const auto dllPath = getDLLPath();
	if (!dllPath.isEmpty()) {
		gameDll = std::make_shared<DynamicLibrary>(dllPath.string());
		loadDLL();
	}
}

Project::~Project()
{
	gameResources.reset();
	gameDll.reset();
	assetImporter.reset();
	plugins.clear();
}

void Project::setPlugins(std::vector<HalleyPluginPtr> plugins)
{
	if (plugins != this->plugins || !assetImporter) {
		this->plugins = std::move(plugins);
		assetImporter = std::make_shared<AssetImporter>(*this, std::vector<Path>{getSharedAssetsSrcPath(), getAssetsSrcPath()});
	}
}

void Project::update(Time time)
{
	withDLL([&] (DynamicLibrary& dll)
	{
		dll.reloadIfChanged();
	});
}

void Project::onBuildDone()
{
	if (!isDLLLoaded()) {
		loadDLL();
		if (isDLLLoaded()) {
			gameDll->notifyReload();
		}
	}
}

const std::vector<String>& Project::getPlatforms() const
{
	return platforms;
}

const Path& Project::getHalleyRootPath() const
{
	return halleyRootPath;
}

const Path& Project::getRootPath() const
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

ECSData& Project::getECSData()
{
	if (!ecsData) {
		loadECSData();
	}
	return *ecsData;
}

const std::shared_ptr<AssetImporter>& Project::getAssetImporter() const
{
	return assetImporter;
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

void Project::addAssetLoadedListener(IAssetLoadListener* listener)
{
	assetLoadedListeners.push_back(std::move(listener));
}

void Project::removeAssetLoadedListener(IAssetLoadListener* listener)
{
	assetLoadedListeners.erase(std::remove(assetLoadedListeners.begin(), assetLoadedListeners.end(), listener), assetLoadedListeners.end());
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
	const auto config = metadata.toConfig();
	const auto str = YAMLConvert::generateYAML(config, {});
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

void Project::onAllAssetsImported()
{
	for (auto& listener: assetLoadedListeners) {
		listener->onAssetsLoaded();
	}
}

void Project::reloadAssets(const std::set<String>& assets, bool packed)
{
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

void Project::reloadCodegen()
{
	loadECSData();
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

constexpr static const char* getDLLExtension()
{
#if defined (_WIN32)
	return ".dll";
#elif defined(__APPLE__)
	return ".dylib";
#else
	return ".so";
#endif	
}

constexpr static const char* getExecutableExtension()
{
#if defined (_WIN32)
	return ".exe";
#else
	return "";
#endif	
}

Path Project::getDLLPath() const
{
	const auto& binName = getProperties().getBinName();
	if (binName.isEmpty()) {
		return "";
	}
	const String suffix = Debug::isDebug() ? "-dll_d" : "-dll";
	return rootPath / "bin" / (binName + suffix + getDLLExtension());
}

void Project::loadDLL()
{
	if (gameDll) {
		gameDll->clearTempDirectory();
		if (gameDll->load(true)) {
			Logger::logInfo("Loaded " + getDLLPath().string());
		}
	}
}

Path Project::getExecutablePath() const
{
	const auto& binName = getProperties().getBinName();
	if (binName.isEmpty()) {
		return "";
	}
	const String suffix = Debug::isDebug() ? "_d" : "";
	return rootPath / "bin" / (binName + suffix + getExecutableExtension());
}

void Project::loadGameResources(const HalleyAPI& api)
{
	auto locator = std::make_unique<ResourceLocator>(*api.system);
	try {
		locator->addFileSystem(getUnpackedAssetsPath());
	} catch (...) {}

	gameResources = std::make_unique<Resources>(std::move(locator), api, Resources::Options(true));
	StandardResources::initialize(*gameResources);
}

Resources& Project::getGameResources()
{
	return *gameResources;
}

bool Project::isDLLLoaded() const
{
	return gameDll && gameDll->isLoaded();
}

std::unique_ptr<Game> Project::createGameInstance() const
{
	if (!isDLLLoaded()) {
		return {};
	}

	const auto getHalleyEntry = reinterpret_cast<IHalleyEntryPoint * (HALLEY_STDCALL*)()>(gameDll->getFunction("getHalleyEntry"));
	if (!getHalleyEntry) {
		return {};
	}
	auto entry = getHalleyEntry();
	
	if (entry->getApiVersion() != HALLEY_DLL_API_VERSION) {
		gameDll->unload();
		return {};
	}
	
	return entry->createGame();
}

void Project::loadECSData()
{
	if (!ecsData) {
		ecsData = std::make_unique<ECSData>();
	} else {
		ecsData->clear();
	}

	const auto& inputFiles = codegenDatabase->getInputFiles();
	const auto n = inputFiles.size();
	std::vector<CodegenSourceInfo> sources(n);
	std::vector<Bytes> inputData(n);
	
	for (size_t i = 0; i < n; ++i) {
		inputData[i] = FileSystem::readFile(getGenSrcPath() / inputFiles[i]);
		auto data = gsl::as_bytes(gsl::span<Byte>(inputData[i].data(), inputData[i].size()));
		sources[i] = CodegenSourceInfo{inputFiles[i], data, true};
	}
	
	ecsData->loadSources(sources);
}

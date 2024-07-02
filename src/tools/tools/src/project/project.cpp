#include <utility>
#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/project/project.h"

#include "halley/api/halley_api.h"
#include "halley/tools/file/filesystem.h"
#include "halley/game/halley_statics.h"
#include "halley/tools/project/project_properties.h"
#include "halley/support/logger.h"
#include "halley/devcon/devcon_server.h"
#include "halley/entry/entry_point.h"
#include "halley/resources/resource_locator.h"
#include "halley/resources/standard_resources.h"
#include "halley/support/debug.h"
#include "halley/tools/assets/metadata_importer.h"
#include "halley/tools/ecs/ecs_data.h"
#include "halley/tools/project/project_loader.h"
#include "halley/game/game.h"
#include "halley/properties/game_properties.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/os/os.h"
#include "halley/tools/codegen/codegen.h"
#include "halley/tools/file/filesystem_cache.h"
#include "halley/tools/project/project_comments.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

constexpr static int currentAssetVersion = 157;
constexpr static int currentCodegenVersion = Codegen::currentCodegenVersion;

Project::Project(Path projectRootPath, Path halleyRootPath, Vector<String> disabledPlatforms)
	: rootPath(std::move(projectRootPath))
	, halleyRootPath(std::move(halleyRootPath))
{
	allowPackedAssets = false;
	fileSystemCache = std::make_unique<FileSystemCache>();

	properties = std::make_unique<ProjectProperties>(rootPath / "halley_project" / "properties.yaml");
	comments = std::make_unique<ProjectComments>(rootPath / "halley_project" / "comments");
	gameProperties = std::make_unique<GameProperties>(rootPath / "assets_src" / "game_properties" / "game_properties.yaml");
	assetPackManifest = rootPath / properties->getAssetPackManifest();

	platforms = properties->getPlatforms();
	for (const auto& plat: disabledPlatforms) {
		std_ex::erase(platforms, plat);
	}

	importAssetsDatabase = std::make_unique<ImportAssetsDatabase>(getUnpackedAssetsPath(), getUnpackedAssetsPath() / "import.db", getUnpackedAssetsPath() / "assets.db", platforms, currentAssetVersion);
	codegenDatabase = std::make_unique<ImportAssetsDatabase>(getGenPath(), getGenPath() / "import.db", getGenPath() / "assets.db", Vector<String>{ "" }, currentCodegenVersion + currentAssetVersion);
	sharedCodegenDatabase = std::make_unique<ImportAssetsDatabase>(getSharedGenPath(), getSharedGenPath() / "import.db", getSharedGenPath() / "assets.db", Vector<String>{ "" }, currentCodegenVersion + currentAssetVersion);
}

Project::~Project()
{
	clearCachedAssetPreviews();
	editorData.reset();
	gameResources.reset();
	gameDll.reset();
	assetImporter.reset();
	plugins.clear();
}

void Project::loadDLL(const HalleyStatics& statics, bool load)
{
	const auto dllPath = getDLLPath();
	if (!dllPath.isEmpty()) {
		gameDll = std::make_shared<ProjectDLL>(dllPath, statics);
		if (load && isBuildSourceUpToDate()) {
			gameDll->load();
		}
	}
}

void Project::setupImporter(Vector<HalleyPluginPtr> plugins, const ConfigNode& importerOptions)
{
	if (plugins != this->plugins || !assetImporter) {
		this->plugins = std::move(plugins);
		assetImporter = std::make_shared<AssetImporter>(*this, Vector<Path>{getSharedAssetsSrcPath(), getAssetsSrcPath()}, ConfigNode(importerOptions));
	}
}

void Project::update(Time time)
{
	withDLL([&] (ProjectDLL& dll)
	{
		dll.reloadIfChanged();
	});

	if (comments->isLoaded()) {
		comments->update(time);
	}
}

bool Project::isBuildPending() const
{
	return buildPending;
}

void Project::onBuildNeeded()
{
	buildPending = true;
}

void Project::onBuildStarted()
{
	buildPending = true;
}

void Project::onBuildDone()
{
	buildCount++;
	Concurrent::execute(Executors::getMainUpdateThread(), [=] () {
		if (gameDll && !gameDll->isLoaded()) {
			gameDll->load();
		}
	});
	buildPending = false;
}

void Project::setPlatforms(Vector<String> platforms)
{
	this->platforms = std::move(platforms);
	importAssetsDatabase->setPlatforms(this->platforms);
}

const Vector<String>& Project::getPlatforms() const
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

Path Project::getEditorAssetsSrcPath() const
{
	return halleyRootPath / "assets_src";
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

ImportAssetType Project::getImportAssetType(const Path& filePath)
{
	return assetImporter->getImportAssetType(filePath, false);
}

const std::shared_ptr<AssetImporter>& Project::getAssetImporter() const
{
	return assetImporter;
}

Vector<std::unique_ptr<IAssetImporter>> Project::getAssetImportersFromPlugins(ImportAssetType type) const
{
	Vector<std::unique_ptr<IAssetImporter>> result;
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
	devConServer = server;
	addAssetPackReloadCallback([this] (gsl::span<const String> assetIds, gsl::span<const String> packIds) {
		devConServer->reloadAssets(Vector<String>(assetIds.begin(), assetIds.end()), Vector<String>(packIds.begin(), packIds.end()));
	});
}

DevConServer* Project::getDevConServer() const
{
	return devConServer;
}

size_t Project::addAssetReloadCallback(AssetReloadCallback callback)
{
	assetReloadCallbacks.emplace_back(++callbackIdx, std::move(callback));
	return callbackIdx;
}

size_t Project::addAssetPackReloadCallback(AssetPackedReloadCallback callback)
{
	assetPackedReloadCallbacks.emplace_back(++callbackIdx, std::move(callback));
	return callbackIdx;
}

void Project::removeAssetReloadCallback(size_t idx)
{
	// Don't erase immediately as this can be called whilst iterating this list
	for (auto& callback: assetReloadCallbacks) {
		if (callback.first == idx) {
			callback.second = {};
		}
	}
}

void Project::removeAssetPackReloadCallback(size_t idx)
{
	for (auto& callback: assetPackedReloadCallbacks) {
		if (callback.first == idx) {
			callback.second = {};
		}
	}
}

void Project::addAssetLoadedListener(IAssetLoadListener* listener)
{
	assetLoadedListeners.push_back(std::move(listener));
}

void Project::removeAssetLoadedListener(IAssetLoadListener* listener)
{
	assetLoadedListeners.erase(std::remove(assetLoadedListeners.begin(), assetLoadedListeners.end(), listener), assetLoadedListeners.end());
}

void Project::notifyAssetsSrcChanged()
{
	for (auto& listener: assetSrcChangeListeners) {
		listener->onAssetsSrcChanged();
	}
}

void Project::notifyGenSrcChanged()
{
	for (auto& listener: assetSrcChangeListeners) {
		listener->onGenSrcChanged();
	}
}

void Project::addAssetSrcChangeListener(IAssetSrcChangeListener& listener)
{
	if (!std_ex::contains(assetSrcChangeListeners, &listener)) {
		assetSrcChangeListeners.push_back(&listener);
	}
}

void Project::removeAssetSrcChangeListener(IAssetSrcChangeListener& listener)
{
	std_ex::erase(assetSrcChangeListeners, &listener);
}

ProjectProperties& Project::getProperties() const
{
	return *properties;
}

ProjectComments& Project::getComments() const
{
	comments->waitForLoad();
	return *comments;
}

GameProperties& Project::getGameProperties() const
{
	return *gameProperties;
}

Metadata Project::readMetadataFromDisk(const Path& filePath) const
{
	Metadata metadata;
	const Path metaPath = filePath.replaceExtension(filePath.getExtension() + ".meta");
	MetadataImporter::loadMetaData(metadata, metaPath, false, filePath);
	return metadata;
}

void Project::writeMetadataToDisk(const Path& filePath, const Metadata& metadata)
{
	const auto config = metadata.toConfig();
	const auto str = YAMLConvert::generateYAML(config, {});
	auto data = Bytes(str.size());
	memcpy(data.data(), str.c_str(), str.size());

	const Path metaPath = filePath.replaceExtension(filePath.getExtension() + ".meta");
	fileSystemCache->writeFile(metaPath, data);
	notifyAssetFilesModified(gsl::span<const Path>(&filePath, 1));
}

void Project::setAssetSaveNotification(bool enabled)
{
	assetNotifyImportEnabled = enabled;
	if (enabled) {
		notifyAssetFilesModified(assetsToNotifyImport);
		assetsToNotifyImport.clear();
	}
}

bool Project::isAssetSaveNotificationEnabled() const
{
	return assetNotifyImportEnabled;
}

bool Project::writeAssetToDisk(const Path& path, gsl::span<const gsl::byte> data)
{
	const Path filePath = getAssetsSrcPath() / path;
	auto existing = fileSystemCache->readFile(filePath);
	auto oldData = gsl::as_bytes(gsl::span<const Byte>(existing));
	
	if (!std::equal(oldData.begin(), oldData.end(), data.begin(), data.end())) {
		fileSystemCache->writeFile(filePath, data);
		if (assetNotifyImportEnabled) {
			notifyAssetFilesModified(gsl::span<const Path>(&path, 1));
		} else {
			assetsToNotifyImport.push_back(path);
		}
		return true;
	}
	return false;
}

bool Project::writeAssetToDisk(const Path& filePath, const Bytes& data)
{
	return writeAssetToDisk(filePath, gsl::as_bytes(gsl::span<const Byte>(data)));
}

bool Project::writeAssetToDisk(const Path& filePath, std::string_view str)
{
	return writeAssetToDisk(filePath, gsl::as_bytes(gsl::span<const char>(str)));
}

Bytes Project::readAssetFromDisk(const Path& filePath)
{
	return fileSystemCache->readFile(getAssetsSrcPath() / filePath);
}

Vector<String> Project::getAssetSrcList(bool includeDirs, const Path& relPath, bool recursive) const
{
	Vector<String> result;
	const auto srcRoot = getAssetsSrcPath();
	const auto dir = srcRoot / relPath;
	fileSystemCache->trackDirectory(srcRoot);
	for (const auto& p: fileSystemCache->enumerateDirectory(dir, includeDirs, recursive)) {
		if (!p.getExtension().endsWith(".meta")) {
			result.push_back((relPath / p).toString());
		}
	}
	return result;
}

Vector<std::pair<AssetType, String>> Project::getAssetsFromFile(const Path& path) const
{
	return importAssetsDatabase->getAssetsFromFile(path);
}

void Project::onAllAssetsImported()
{
	for (auto& listener: assetLoadedListeners) {
		listener->onAssetsLoaded();
	}
	assetsImported = true;
}

void Project::reloadAssets(const std::set<String>& assets, const Vector<String>& packIds, bool packed)
{
	// Build name list
	Vector<String> assetIds;
	assetIds.reserve(assets.size());
	for (auto& a: assets) {
		assetIds.push_back(a);
	}

	// Reload game assets
	if (packed && gameResources) {
		if (gameResources->getLocator().getLocatorCount() == 0) {
			try {
				if (allowPackedAssets && getGameInstance()) {
					getGameInstance()->initResourceLocator(rootPath, getPackedAssetsPath("pc"), getUnpackedAssetsPath(), gameResources->getLocator());
				} else {
					gameResources->getLocator().addFileSystem(getUnpackedAssetsPath());
				}
			} catch (...) {}
		}

		gameResources->reloadAssets(assetIds, packIds);
	}

	// Erase any callbacks that got deleted (i.e. set to empty), then call remaining ones
	std_ex::erase_if(assetPackedReloadCallbacks, [&] (const auto& c)
	{
		return !c.second;
	});
	std_ex::erase_if(assetReloadCallbacks, [&] (const auto& c)
	{
		return !c.second;
	});
	if (packed) {
		for (auto& callback : assetPackedReloadCallbacks) {
			// Can be set to empty in the middle of this loop, so this check is necessary despite the removal of empty ones above
			if (callback.second) {
				callback.second(assetIds, packIds);
			}
		}
	} else {
		for (auto& callback : assetReloadCallbacks) {
			// Can be set to empty in the middle of this loop, so this check is necessary despite the removal of empty ones above
			if (callback.second) {
				callback.second(assetIds);
			}
		}
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

void Project::notifyAssetFilesModified(gsl::span<const Path> paths)
{
	if (checkAssetsTask) {
		checkAssetsTask->requestRefreshAssets(paths);
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
	auto* game = getGameInstance();
	this->api = &api;

	try {
		if (allowPackedAssets && game) {
			game->initResourceLocator(rootPath, getPackedAssetsPath("pc"), getUnpackedAssetsPath(), *locator);
		} else {
			locator->addFileSystem(getUnpackedAssetsPath(), fileSystemCache.get());
		}
	} catch (...) {}

	gameResources = std::make_unique<Resources>(std::move(locator), api, ResourceOptions(true));
	StandardResources::initialize(*gameResources);
	loadGameEditorData();
}

Resources& Project::getGameResources()
{
	return *gameResources;
}

bool Project::isDLLLoaded() const
{
	return gameDll && gameDll->isLoaded();
}

bool Project::areAssetsLoaded() const
{
	return assetsImported;
}

ProjectDLL::Status Project::getDLLStatus() const
{
	return gameDll ? gameDll->getStatus() : ProjectDLL::Status::Unloaded;
}

Game* Project::getGameInstance() const
{
	Game* result = nullptr;
	withLoadedDLL([&] (ProjectDLL& dll)
	{
		result = &dll.getGame();
	});
	return result;
}

std::optional<AssetPreviewData> Project::getCachedAssetPreview(AssetType type, const String& id)
{
	if (const auto previewIter = previewCache.find(std::pair(type, id)); previewIter != previewCache.end()) {
		const auto& cache = previewIter->second;
		const auto assetTimestamp = getImportAssetsDatabase().getAssetTimestamp(type, id);
		if (cache.timestamp == assetTimestamp) {
			return cache.data;
		}
	}
	return {};
}

void Project::setCachedAssetPreview(AssetType type, const String& id, AssetPreviewData data)
{
	const auto assetTimestamp = getImportAssetsDatabase().getAssetTimestamp(type, id);
	previewCache[std::pair(type, id)] = AssetPreviewCache{ assetTimestamp, std::move(data) };
}

void Project::clearCachedAssetPreviews()
{
	previewCache.clear();
}

FileSystemCache& Project::getFileSystemCache()
{
	return *fileSystemCache;
}

void Project::requestReimport(ReimportType reimport)
{
	if (checkAssetsTask) {
		checkAssetsTask->requestReimport(reimport);
	}
}

void Project::launchGame(Vector<String> params) const
{
	const String args = String::concatList(params, " ");
	OS::get().runCommandAsync("\"" + getExecutablePath().getNativeString() + "\" " + args, getExecutablePath().parentPath().getNativeString());
}

uint64_t Project::getSourceHash(const Path& projectRoot)
{
	const auto srcRoot = projectRoot / "src";

	Hash::Hasher hasher;
	auto files = FileSystem::enumerateDirectory(srcRoot);
	std::sort(files.begin(), files.end());
	for (const auto& file: files) {
		hasher.feed(FileSystem::getLastWriteTime(srcRoot / file));
	}
	return hasher.digest();
}

uint64_t Project::getSourceHash() const
{
	return getSourceHash(rootPath);
}

String Project::getBuiltSourceStr() const
{
	return Path::readFileString(rootPath / "bin" / "code_version.txt");
}

bool Project::isBuildSourceUpToDate() const
{
	return getBuiltSourceStr() == toString(getSourceHash(), 16);
}

uint32_t Project::getBuildCount() const
{
	return buildCount;
}

Vector<Path> Project::enumerateDirectory(const Path& path)
{
	return FileSystem::enumerateDirectory(path);
}

void Project::onDLLLoaded()
{
	loadGameEditorData();
}

void Project::onDLLUnload()
{
	for (const auto& ss: getGameResources().enumerate<SpriteSheet>()) {
		getGameResources().get<SpriteSheet>(ss)->clearMaterialCache();
	}
	clearCachedAssetPreviews();
	editorData = {};
	editorDataLoading = {};
}

void Project::loadECSData()
{
	if (!ecsData) {
		ecsData = std::make_unique<ECSData>();
	} else {
		ecsData->clear();
	}

	const auto& inputFiles = codegenDatabase->getAllInputFiles();
	const auto n = inputFiles.size();
	Vector<CodegenSourceInfo> sources(n);
	Vector<Bytes> inputData(n);
	
	for (size_t i = 0; i < n; ++i) {
		inputData[i] = fileSystemCache->readFile(getGenSrcPath() / inputFiles[i]);
		auto data = gsl::as_bytes(gsl::span<Byte>(inputData[i].data(), inputData[i].size()));
		sources[i] = CodegenSourceInfo{inputFiles[i], data, true};
	}
	
	ecsData->loadSources(sources, false);
}

void Project::loadGameEditorData() const
{
	if (editorData || editorDataLoading.isValid()) {
		return;
	}

	if (auto game = getGameInstance()) {
		editorDataLoading = Concurrent::execute([this, game] ()
		{
			editorData = game->createGameEditorData(*api, *gameResources);
		});
	}
}

IGameEditorData* Project::getGameEditorData() const
{
	loadGameEditorData();

	if (editorDataLoading.isValid()) {
		editorDataLoading.wait();
	}
	return editorData.get();
}

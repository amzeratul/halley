#include <set>
#include <thread>
#include "halley/tools/assets/check_assets_task.h"
#include "halley/tools/assets/import_assets_task.h"
#include "halley/tools/project/project.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/assets/delete_assets_task.h"
#include <boost/filesystem/operations.hpp>
#include "halley/tools/file/filesystem.h"
#include "halley/support/logger.h"
#include "../yaml/halley-yamlcpp.h"
#include "halley/resources/resource_data.h"

using namespace Halley;
using namespace std::chrono_literals;

CheckAssetsTask::CheckAssetsTask(Project& project, bool oneShot)
	: EditorTask("Check assets", true, false)
	, project(project)
	, monitorAssets(project.getUnpackedAssetsPath())
	, monitorAssetsSrc(project.getAssetsSrcPath())
	, monitorSharedAssetsSrc(project.getSharedAssetsSrcPath())
	, monitorGen(project.getGenPath())
	, monitorGenSrc(project.getGenSrcPath())
	, oneShot(oneShot)
{}

void CheckAssetsTask::run()
{
	bool first = true;
	while (!isCancelled()) {
		if (first | monitorAssets.poll() | monitorAssetsSrc.poll() | monitorSharedAssetsSrc.poll()) { // Don't short-circuit
			Logger::logInfo("Scanning for asset changes...");
			checkAllAssets(project.getImportAssetsDatabase(), { project.getAssetsSrcPath(), project.getSharedAssetsSrcPath() }, project.getUnpackedAssetsPath(), "Importing assets", true);
		}

		if (first | monitorGen.poll() | monitorGenSrc.poll()) {
			Logger::logInfo("Scanning for codegen changes...");
			checkAllAssets(project.getCodegenDatabase(), { project.getGenSrcPath() }, project.getGenPath(), "Generating code", false);
		}

		first = false;

		while (hasPendingTasks()) {
			std::this_thread::sleep_for(5ms);
		}

		if (oneShot) {
			return;
		}
		std::this_thread::sleep_for(monitorAssets.hasRealImplementation() ? 100ms : 1000ms);
	}
}

static void loadMetaTable(Metadata& meta, const YAML::Node& root)
{
	for (YAML::const_iterator it = root.begin(); it != root.end(); ++it) {
		String key = it->first.as<std::string>();
		String value = it->second.as<std::string>();
		meta.set(key, value);
	}
}

static void loadMetaData(Metadata& meta, const Path& path, bool isDirectoryMeta, String assetId)
{
	auto data = ResourceDataStatic::loadFromFileSystem(path);
	auto root = YAML::Load(data->getString());

	if (isDirectoryMeta) {
		for (const auto& rootList: root) {
			bool matches = true;
			for (YAML::const_iterator i0 = rootList.begin(); i0 != rootList.end(); ++i0) {
				auto name = i0->first.as<std::string>();
				if (name == "match") {
					matches = false;
					for (auto& pattern: i0->second) {
						if (assetId.contains(pattern.as<std::string>())) {
							matches = true;
							break;
						}
					}
				} else if (name == "data" && matches) {
					loadMetaTable(meta, i0->second);
					return;
				}
			}
		}
	} else {
		loadMetaTable(meta, root);
	}
}

static Metadata getMetaData(Path inputFilePath, Maybe<Path> dirMetaPath, Maybe<Path> privateMetaPath)
{
	Metadata meta;
	try {
		if (dirMetaPath) {
			loadMetaData(meta, dirMetaPath.get(), true, inputFilePath.toString());
		}
		if (privateMetaPath) {
			loadMetaData(meta, privateMetaPath.get(), false, inputFilePath.toString());
		}
	} catch (std::exception& e) {
		throw Exception("Error parsing metafile for " + inputFilePath + ": " + e.what(), HalleyExceptions::Tools);
	}
	return meta;
}

bool CheckAssetsTask::importFile(ImportAssetsDatabase& db, std::map<String, ImportAssetsDatabaseEntry>& assets, const bool isCodegen, const std::vector<Path>& directoryMetas, const Path& srcPath, const Path& filePath) {
	std::array<int64_t, 3> timestamps = {{ 0, 0, 0 }};
	bool dbChanged = false;

	// Collect data on main file
	timestamps[0] = FileSystem::getLastWriteTime(srcPath / filePath);

	// Collect data on directory meta file
	auto dirMetaPath = findDirectoryMeta(directoryMetas, filePath);
	if (dirMetaPath && FileSystem::exists(srcPath / dirMetaPath.get())) {
		dirMetaPath = srcPath / dirMetaPath.get();
		timestamps[1] = FileSystem::getLastWriteTime(dirMetaPath.get());
	} else {
		dirMetaPath = {};
	}

	// Collect data on private meta file
	Maybe<Path> privateMetaPath = srcPath / filePath.replaceExtension(filePath.getExtension() + ".meta");
	if (FileSystem::exists(privateMetaPath.get())) {
		timestamps[2] = FileSystem::getLastWriteTime(privateMetaPath.get());
	} else {
		privateMetaPath = {};
	}

	// Load metadata if needed
	if (db.needToLoadInputMetadata(filePath, timestamps)) {
		Metadata meta = getMetaData(filePath, dirMetaPath, privateMetaPath);
		db.setInputFileMetadata(filePath, timestamps, meta);
		dbChanged = true;
	}

	// Figure out the right importer and assetId for this file
	auto& assetImporter = isCodegen ? project.getAssetImporter().getImporter(ImportAssetType::Codegen) : project.getAssetImporter().getImporter(filePath);
	if (assetImporter.getType() == ImportAssetType::Skip) {
		return false;
	}
	String assetId = assetImporter.getAssetId(filePath, db.getMetadata(filePath));

	// Build timestamped path
	auto input = TimestampedPath(filePath, std::max(timestamps[0], std::max(timestamps[1], timestamps[2])));

	// Build the asset
	auto iter = assets.find(assetId);
	if (iter == assets.end()) {
		// New; create it
		auto& asset = assets[assetId];
		asset.assetId = assetId;
		asset.assetType = assetImporter.getType();
		asset.srcDir = srcPath;
		asset.inputFiles.push_back(input);
	} else {
		// Already exists
		auto& asset = iter->second;
		if (asset.assetType != assetImporter.getType()) { // Ensure it has the correct type
			throw Exception("AssetId conflict on " + assetId, HalleyExceptions::Tools);
		}
		if (asset.srcDir == srcPath) { // Don't mix files from two different source paths
			asset.inputFiles.push_back(input);
		} else {
			throw Exception("Mixed source dir input for " + assetId, HalleyExceptions::Tools);
		}
	}

	return dbChanged;
}

void CheckAssetsTask::checkAllAssets(ImportAssetsDatabase& db, std::vector<Path> srcPaths, Path dstPath, String taskName, bool packAfter)
{
	std::map<String, ImportAssetsDatabaseEntry> assets;

	bool isCodegen = srcPaths.size() == 1 && srcPaths[0] == project.getGenSrcPath();
	bool dbChanged = false;

	std::vector<Path> directoryMetas;

	// Enumerate all potential assets
	for (auto srcPath : srcPaths) {
		auto allFiles = FileSystem::enumerateDirectory(srcPath);

		// First, collect all directory metas
		for (auto& filePath : allFiles) {
			if (filePath.getFilename() == "_dir.meta") {
				directoryMetas.push_back(filePath);
			}
		}

		// Next, go through normal files
		for (auto& filePath : allFiles) {
			if (filePath.getExtension() == ".meta") {
				continue;
			}

			dbChanged = dbChanged | importFile(db, assets, isCodegen, directoryMetas, srcPath, filePath);
		}
	}

	if (dbChanged) {
		db.save();
	}

	// Check for missing input files
	db.markAssetsAsStillPresent(assets);
	auto toDelete = db.getAllMissing();
	std::vector<String> deletedAssets;
	if (!toDelete.empty()) {
		for (auto& a: toDelete) {
			for (auto& out: a.outputFiles) {
				deletedAssets.push_back(toString(out.type) + ":" + out.name);
			}
		}

		Logger::logInfo("Assets to be deleted: " + toString(toDelete.size()));
		addPendingTask(EditorTaskAnchor(std::make_unique<DeleteAssetsTask>(db, dstPath, std::move(toDelete))));
	}

	// Import assets
	auto toImport = filterNeedsImporting(db, assets);
	if (!toImport.empty() || !deletedAssets.empty()) {
		Logger::logInfo("Assets to be imported: " + toString(toImport.size()));
		addPendingTask(EditorTaskAnchor(std::make_unique<ImportAssetsTask>(taskName, db, project.getAssetImporter(), dstPath, std::move(toImport), std::move(deletedAssets), project, packAfter)));
	}
}

std::vector<ImportAssetsDatabaseEntry> CheckAssetsTask::filterNeedsImporting(ImportAssetsDatabase& db, const std::map<String, ImportAssetsDatabaseEntry>& assets)
{
	Vector<ImportAssetsDatabaseEntry> toImport;

	for (auto& a: assets) {
		if (db.needsImporting(a.second)) {
			toImport.push_back(a.second);
		}
	}

	return toImport;
}

Maybe<Path> CheckAssetsTask::findDirectoryMeta(const std::vector<Path>& metas, const Path& path) const
{
	auto parent = path.parentPath();
	for (auto& m: metas) {
		auto n = m.getNumberPaths() - 1;
		if (m.getFront(n) == parent.getFront(n)) {
			return m;
		}
	}
	return {};
}

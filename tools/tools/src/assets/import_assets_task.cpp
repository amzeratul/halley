#include <thread>
#include "halley/tools/assets/import_assets_task.h"
#include "halley/tools/assets/check_assets_task.h"
#include "halley/tools/project/project.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/resources/resource_data.h"
#include <yaml-cpp/yaml.h>
#include "halley/tools/file/filesystem.h"

using namespace Halley;

ImportAssetsTask::ImportAssetsTask(String taskName, ImportAssetsDatabase& db, const AssetImporter& importer, Path assetsPath, Vector<ImportAssetsDatabaseEntry>&& files)
	: EditorTask(taskName, true, true)
	, db(db)
	, importer(importer)
	, assetsPath(assetsPath)
	, files(std::move(files))
{}

void ImportAssetsTask::run()
{
	using namespace std::chrono_literals;
	auto lastSave = std::chrono::steady_clock::now();

	for (size_t i = 0; i < files.size(); ++i) {
		if (isCancelled()) {
			break;
		}

		curFileProgressStart = float(i) / float(files.size());
		curFileProgressEnd = float(i + 1) / float(files.size());
		curFileLabel = files[i].assetId;
		setProgress(curFileProgressStart, curFileLabel);

		if (importAsset(files[i])) {
			// Check if db needs saving
			auto now = std::chrono::steady_clock::now();
			if (now - lastSave > 1s) {
				db.save();
				lastSave = now;
			}
		}
	}
	db.save();

	if (!isCancelled()) {
		setProgress(1.0f, "");
	}
}

static std::unique_ptr<Metadata> getMetaData(const ImportAssetsDatabaseEntry& asset)
{
	try {
		if (asset.inputFiles.size() > 1) {
			for (auto& i: asset.inputFiles) {
				if (i.first.getExtension() == ".meta") {
					auto data = ResourceDataStatic::loadFromFileSystem(asset.srcDir / i.first);
					auto root = YAML::Load(data->getString());

					auto meta = std::make_unique<Metadata>();

					for (YAML::const_iterator it = root.begin(); it != root.end(); ++it) {
						String key = it->first.as<std::string>();
						String value = it->second.as<std::string>();
						meta->set(key, value);
					}

					return meta;
				}
			}
		}
		return std::unique_ptr<Metadata>();
	} catch (std::exception& e) {
		throw Exception("Error parsing metafile for " + asset.assetId + ": " + e.what());
	}
}

bool ImportAssetsTask::importAsset(ImportAssetsDatabaseEntry& asset)
{
	std::vector<Path> out;
	try {
		// Create queue
		std::list<ImportingAsset> toLoad;

		// Load files from disk
		ImportingAsset importingAsset;
		importingAsset.assetId = asset.assetId;
		importingAsset.assetType = asset.assetType;
		importingAsset.metadata = getMetaData(asset);
		for (auto& f: asset.inputFiles) {
			if (f.first.getExtension() != ".meta") {
				importingAsset.inputFiles.emplace_back(ImportingAssetFile(f.first, FileSystem::readFile(asset.srcDir / f.first)));
			}
		}
		if (importingAsset.inputFiles.empty()) {
			throw Exception("No input files found for asset " + asset.assetId + " - do you have a stray meta file?");
		}
		toLoad.emplace_back(std::move(importingAsset));

		// Import
		while (!toLoad.empty()) {
			auto cur = std::move(toLoad.front());
			toLoad.pop_front();
			std::cout << "Import: " << cur.assetId << std::endl;
			std::vector<Path> curOut = importer.getImporter(cur.assetType).import(cur, assetsPath, [&] (float progress, const String& label) -> bool
			{
				setProgress(lerp(curFileProgressStart, curFileProgressEnd, progress), curFileLabel + " " + label);
				return !isCancelled();
			}, [&] (ImportingAsset&& toAdd)
			{
				toLoad.emplace_front(std::move(toAdd));
			});

			for (auto& o: curOut) {
				out.push_back(o);
			}
		}
	} catch (std::exception& e) {
		addError("\"" + asset.assetId + "\" - " + e.what());
		db.markFailed(asset);

		return false;
	}

	// Check if it didn't get cancelled
	if (isCancelled()) {
		return false;
	}

	// Retrieve previous output from this asset, and remove any files which went missing
	auto previous = db.getOutFiles(asset.assetId);
	for (auto& f: previous) {
		if (std::find(out.begin(), out.end(), f) == out.end()) {
			// File no longer exists as part of this asset, remove it
			FileSystem::remove(assetsPath / f);
		}
	}

	// Store output in db
	asset.outputFiles = out;
	db.markAsImported(asset);

	return true;
}

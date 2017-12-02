#include <thread>
#include "halley/tools/assets/import_assets_task.h"
#include "halley/tools/assets/check_assets_task.h"
#include "halley/tools/project/project.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/resources/resource_data.h"
#include "../yaml/halley-yamlcpp.h"
#include "halley/tools/file/filesystem.h"
#include "halley/tools/assets/asset_collector.h"

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
				}
			}
		}
	} else {
		loadMetaTable(meta, root);
	}
}

static std::unique_ptr<Metadata> getMetaData(const ImportAssetsDatabaseEntry& asset)
{
	try {
		auto meta = std::make_unique<Metadata>();

		for (int j = 0; j < 2; ++j) {
			// First load the directory meta, then load private meta on top of it, so it overrides
			bool isDirectoryMeta = j == 0;

			for (auto& i: asset.inputFiles) {
				if (i.first.getExtension() == ".meta" && (i.first.getFilename() == "_dir.meta") == isDirectoryMeta) {
					loadMetaData(*meta, asset.srcDir / i.first, isDirectoryMeta, asset.assetId);
				}
			}
		}
		
		return meta;
	} catch (std::exception& e) {
		throw Exception("Error parsing metafile for " + asset.assetId + ": " + e.what());
	}
}

bool ImportAssetsTask::importAsset(ImportAssetsDatabaseEntry& asset)
{
	std::vector<AssetResource> out;
	std::vector<TimestampedPath> additionalInputs;
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
			
			AssetCollector collector(cur, assetsPath, importer.getAssetsSrc(), [=] (float assetProgress, const String& label) -> bool
			{
				setProgress(lerp(curFileProgressStart, curFileProgressEnd, assetProgress), curFileLabel + " " + label);
				return !isCancelled();
			});
			
			importer.getImporter(cur.assetType).import(cur, collector);
			
			for (auto& additional: collector.collectAdditionalAssets()) {
				toLoad.emplace_front(std::move(additional));
			}

			for (auto& o: collector.getAssets()) {
				out.push_back(o);
			}

			for (auto& i: collector.getAdditionalInputs()) {
				additionalInputs.push_back(i);
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
		if (std::find_if(out.begin(), out.end(), [&] (const AssetResource& r) { return r.filepath == f.filepath; }) == out.end()) {
			// File no longer exists as part of this asset, remove it
			FileSystem::remove(assetsPath / f.filepath);
		}
	}

	// Store output in db
	asset.additionalInputFiles = std::move(additionalInputs);
	asset.outputFiles = std::move(out);
	db.markAsImported(asset);

	return true;
}

#include <thread>
#include "halley/tools/assets/import_assets_task.h"
#include "halley/tools/assets/check_assets_task.h"
#include "halley/tools/project/project.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/resources/resource_data.h"
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
		for (auto& f: asset.inputFiles) {
			auto meta = db.getMetadata(f.first);
			if (meta) {
				if (importingAsset.metadata) {
					// Already has metadata, ensure they're compatible
					if (*importingAsset.metadata != meta.get()) {
						throw Exception("Asset \"" + asset.assetId + "\" has different files with incompatible metadata");
					}
				} else {
					importingAsset.metadata = std::make_unique<Metadata>(meta.get());
				}
			}

			importingAsset.inputFiles.emplace_back(ImportingAssetFile(f.first, FileSystem::readFile(asset.srcDir / f.first)));
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

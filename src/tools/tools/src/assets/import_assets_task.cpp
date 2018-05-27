#include <thread>
#include "halley/tools/assets/import_assets_task.h"
#include "halley/tools/assets/check_assets_task.h"
#include "halley/tools/project/project.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/resources/resource_data.h"
#include "halley/tools/file/filesystem.h"
#include "halley/tools/assets/asset_collector.h"
#include "halley/concurrency/concurrent.h"
#include "halley/tools/packer/asset_packer_task.h"
#include "halley/support/logger.h"
#include "halley/time/stopwatch.h"

using namespace Halley;

ImportAssetsTask::ImportAssetsTask(String taskName, ImportAssetsDatabase& db, const AssetImporter& importer, Path assetsPath, Vector<ImportAssetsDatabaseEntry>&& files, Project& project, bool packAfter)
	: EditorTask(taskName, true, true)
	, db(db)
	, importer(importer)
	, assetsPath(assetsPath)
	, project(project)
	, packAfter(packAfter)
	, files(std::move(files))
{}

void ImportAssetsTask::run()
{
	Stopwatch timer;
	using namespace std::chrono_literals;
	auto lastSave = std::chrono::steady_clock::now();

	assetsImported = 0;
	assetsToImport = files.size();
	std::vector<Future<void>> tasks;

	for (size_t i = 0; i < files.size(); ++i) {
		tasks.push_back(Concurrent::execute(Executors::getCPUAux(), [&, i] () {
			if (isCancelled()) {
				return;
			}

			if (importAsset(files[i])) {
				++assetsImported;
				setProgress(float(assetsImported) * 0.98f / float(assetsToImport), files[i].assetId);
			}

			auto now = std::chrono::steady_clock::now();
			if (now - lastSave > 1s) {
				db.save();
				lastSave = now;
			}
		}));
	}

	Concurrent::whenAll(tasks.begin(), tasks.end()).get();
	db.save();

	if (!isCancelled()) {
		setProgress(1.0f, "");

		if (packAfter) {
			addContinuation(EditorTaskAnchor(std::make_unique<AssetPackerTask>(project, std::move(outputAssets))));
		}
	}

	timer.pause();
	Time realTime = timer.elapsedNanoSeconds() / 1000000000.0;
	Time importTime = totalImportTime / 1000000000.0;
	Logger::logInfo("Import took " + toString(realTime) + " seconds, on which " + toString(importTime) + " seconds of work were performed (" + toString(importTime / realTime) + "x realtime)");
}

bool ImportAssetsTask::importAsset(ImportAssetsDatabaseEntry& asset)
{
	Logger::logInfo("Importing " + asset.assetId);
	Stopwatch timer;

	std::vector<AssetResource> out;
	std::vector<std::pair<Path, Bytes>> outFiles;
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
			importingAsset.inputFiles.emplace_back(ImportingAssetFile(f.first, FileSystem::readFile(asset.srcDir / f.first), meta ? meta.get() : Metadata()));
		}
		toLoad.emplace_back(std::move(importingAsset));

		// Import
		while (!toLoad.empty()) {
			auto cur = std::move(toLoad.front());
			toLoad.pop_front();
			
			AssetCollector collector(cur, assetsPath, importer.getAssetsSrc(), [=] (float assetProgress, const String& label) -> bool
			{
				//setProgress(lerp(curFileProgressStart, curFileProgressEnd, assetProgress), curFileLabel + " " + label);
				return !isCancelled();
			});
			
			importer.getImporter(cur.assetType).import(cur, collector);
			
			for (auto& additional: collector.collectAdditionalAssets()) {
				toLoad.emplace_front(std::move(additional));
			}

			for (auto& outFile: collector.collectOutFiles()) {
				outFiles.push_back(std::move(outFile));
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
		asset.additionalInputFiles = std::move(additionalInputs);
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
		if (std::find_if(outFiles.begin(), outFiles.end(), [&] (const std::pair<Path, Bytes>& r) { return r.first == f.filepath; }) == outFiles.end()) {
			// File no longer exists as part of this asset, remove it
			FileSystem::remove(assetsPath / f.filepath);
		}
	}

	// Write files
	for (auto& outFile: outFiles) {
		auto path = assetsPath / outFile.first;
		Logger::logInfo("- " + asset.assetId + " -> " + path + " (" + String::prettySize(outFile.second.size()) + ")");
		FileSystem::writeFile(path, outFile.second);
	}

	// Add to list of output assets
	for (auto& o: out) {
		outputAssets.insert(toString(o.type) + ":" + o.name);
	}

	// Store output in db
	asset.additionalInputFiles = std::move(additionalInputs);
	asset.outputFiles = std::move(out);
	db.markAsImported(asset);

	timer.pause();
	totalImportTime += timer.elapsedNanoSeconds();

	return true;
}

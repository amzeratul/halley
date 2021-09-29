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
#include "halley/time/stopwatch.h"
#include "halley/support/debug.h"

using namespace Halley;

ImportAssetsTask::ImportAssetsTask(String taskName, ImportAssetsDatabase& db, std::shared_ptr<AssetImporter> importer, Path assetsPath, Vector<ImportAssetsDatabaseEntry> files, std::vector<String> deletedAssets, Project& project, bool packAfter)
	: Task(taskName, true, true)
	, db(db)
	, importer(importer)
	, assetsPath(assetsPath)
	, project(project)
	, packAfter(packAfter)
	, files(std::move(files))
	, deletedAssets(std::move(deletedAssets))
	, totalImportTime(0)
{}

void ImportAssetsTask::run()
{
	Stopwatch timer;
	using namespace std::chrono_literals;
	auto lastSave = std::chrono::steady_clock::now();

	assetsImported = 0;
	assetsToImport = files.size();
	std::vector<Future<void>> tasks;

	constexpr bool parallelImport = !Debug::isDebug();

	for (size_t i = 0; i < files.size(); ++i) {
		auto importFunc = [&, i] () {
			if (isCancelled()) {
				return;
			}

			if (doImportAsset(files[i])) {
				++assetsImported;
				setProgress(float(assetsImported) * 0.98f / float(assetsToImport), files[i].assetId);
			}

			auto now = std::chrono::steady_clock::now();
			if (now - lastSave > 1s) {
				db.save();
				lastSave = now;
			}
		};

		if (parallelImport) {
			tasks.push_back(Concurrent::execute(Executors::getCPUAux(), importFunc));
		} else {
			importFunc();
		}
	}

	Concurrent::whenAll(tasks.begin(), tasks.end()).get();
	db.save();

	if (!isCancelled()) {
		setProgress(1.0f, "");

		if (!hasError()) {
			if (!outputAssets.empty()) {
				Concurrent::execute(Executors::getMainThread(), [project = &project, assets = outputAssets] () {
					project->reloadAssets(assets, false);
				});
			}

			if (files.size() == 1 && files[0].assetId == ":codegen") {
				Concurrent::execute(Executors::getMainThread(), [project = &project]() {
					project->reloadCodegen();
				});
			}

			if (packAfter) {
				addContinuation(std::make_unique<AssetPackerTask>(project, std::move(outputAssets), std::move(deletedAssets)));
			}
		}
	}

	timer.pause();
	Time realTime = timer.elapsedNanoseconds() / 1000000000.0;
	Time importTime = totalImportTime / 1000000000.0;
	logInfo("Import took " + toString(realTime) + " seconds, on which " + toString(importTime) + " seconds of work were performed (" + toString(importTime / realTime) + "x realtime)");
}

bool ImportAssetsTask::doImportAsset(ImportAssetsDatabaseEntry& asset)
{
	logInfo("Importing " + asset.assetId);
	Stopwatch timer;

	auto result = importAsset(asset, [&] (const Path& path) { return db.getMetadata(path); }, *importer, assetsPath, [=] (float, const String&) -> bool { return !isCancelled(); });
	
	if (!result.success) {
		logError("\"" + asset.assetId + "\" - " + result.errorMsg);
		asset.additionalInputFiles = std::move(result.additionalInputs);
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
		for (auto& v: f.platformVersions) {
			if (std::find_if(result.outFiles.begin(), result.outFiles.end(), [&] (const std::pair<Path, Bytes>& r) { return r.first == v.second.filepath; }) == result.outFiles.end()) {
				// File no longer exists as part of this asset, remove it
				FileSystem::remove(assetsPath / v.second.filepath);
			}
		}
	}

	// Write files
	for (auto& outFile: result.outFiles) {
		auto path = assetsPath / outFile.first;
		logInfo("- " + asset.assetId + " -> " + path + " (" + String::prettySize(outFile.second.size()) + ")");
		FileSystem::writeFile(path, outFile.second);
	}

	// Add to list of output assets
	{
		std::unique_lock<std::mutex> lock(mutex);
		for (auto& o: result.out) {
			outputAssets.insert(toString(o.type) + ":" + o.name);
		}
	}

	// Store output in db
	asset.additionalInputFiles = std::move(result.additionalInputs);
	asset.outputFiles = std::move(result.out);
	db.markAsImported(asset);

	timer.pause();
	totalImportTime += timer.elapsedNanoseconds();

	return true;
}

ImportAssetsTask::ImportResult ImportAssetsTask::importAsset(const ImportAssetsDatabaseEntry& asset, const MetadataFetchCallback& metadataFetcher, const AssetImporter& importer, Path assetsPath, AssetCollector::ProgressReporter progressReporter)
{
	ImportResult result;
	
	try {
		// Create queue
		std::list<ImportingAsset> toLoad;

		// Load files from disk
		ImportingAsset importingAsset;
		importingAsset.assetId = asset.assetId;
		importingAsset.assetType = asset.assetType;
		for (const auto& f: asset.inputFiles) {
			auto meta = metadataFetcher(f.getPath());
			auto data = FileSystem::readFile(asset.srcDir / f.getDataPath());
			if (data.empty()) {
				// Give it a bit and try again if it was empty
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(5ms);
				data = FileSystem::readFile(asset.srcDir / f.getDataPath());

				if (data.empty()) {
					logError("Data for \"" + toString(asset.srcDir / f.getPath()) + "\" is empty.");
				}
			}
			importingAsset.inputFiles.emplace_back(ImportingAssetFile(f.getPath(), std::move(data), meta ? meta.value() : Metadata()));
		}
		toLoad.emplace_back(std::move(importingAsset));

		// Import
		while (!toLoad.empty()) {
			auto cur = std::move(toLoad.front());
			toLoad.pop_front();
			
			AssetCollector collector(cur, assetsPath, importer.getAssetsSrc(), progressReporter);

			for (const auto& assetImporter: importer.getImporters(cur.assetType)) {
				try {
					assetImporter.get().import(cur, collector);
				} catch (...) {
					for (const auto& i: collector.getAdditionalInputs()) {
						result.additionalInputs.push_back(i);
					}
					throw;
				}
			}
			
			for (auto& additional: collector.collectAdditionalAssets()) {
				toLoad.emplace_front(std::move(additional));
			}

			for (auto& outFile: collector.collectOutFiles()) {
				result.outFiles.push_back(std::move(outFile));
			}

			for (const auto& o: collector.getAssets()) {
				result.out.push_back(o);
			}

			for (const auto& i: collector.getAdditionalInputs()) {
				result.additionalInputs.push_back(i);
			}
		}
		
		result.success = true;
	} catch (const Exception& e) {
		result.errorMsg = e.getMessage();
		result.success = false;
	} catch (const std::exception& e) {
		result.errorMsg = e.what();
		result.success = false;
	}

	return result;
}

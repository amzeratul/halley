#include <thread>
#include "halley/tools/assets/import_assets_task.h"
#include "halley/tools/assets/check_assets_task.h"
#include "halley/tools/project/project.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/resources/resource_data.h"

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
	std::vector<Path> out;
	try {
		// Import
		out = importer.getImporter(asset.assetType).import(asset, assetsPath, [&] (float progress, String label) -> bool
		{
			setProgress(lerp(curFileProgressStart, curFileProgressEnd, progress), curFileLabel + " " + label);
			return !isCancelled();
		});
	} catch (std::exception& e) {
		std::cout << "Error importing asset " << asset.assetId << ": " << e.what() << std::endl;
		
		// TODO: mark asset as pending fix

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

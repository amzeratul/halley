#pragma once
#include "halley/concurrency/task.h"
#include "halley/file/path.h"
#include "import_assets_database.h"
#include "halley/data_structures/vector.h"
#include <set>

#include "asset_collector.h"

namespace Halley
{
	class Project;
	
	class ImportAssetsTask : public Task
	{
	public:
		struct ImportResult {
			Vector<AssetResource> out;
			Vector<std::pair<Path, Bytes>> outFiles;
			Vector<TimestampedPath> additionalInputs;
			bool success = false;
			String errorMsg;
		};
		using MetadataFetchCallback = std::function<std::optional<Metadata>(const Path&)>;
		
		ImportAssetsTask(String taskName, ImportAssetsDatabase& db, std::shared_ptr<AssetImporter> importer, Path assetsPath, Vector<ImportAssetsDatabaseEntry> files, Vector<String> deletedAssets, Project& project, bool packAfter);

	protected:
		void run() override;

	private:
		ImportAssetsDatabase& db;
		std::shared_ptr<AssetImporter> importer;
		Path assetsPath;
		Project& project;
		const bool packAfter;

		Vector<ImportAssetsDatabaseEntry> files;
		Vector<String> deletedAssets;
		std::set<String> outputAssets;
		
		std::atomic<int64_t> totalImportTime;
		std::atomic<size_t> assetsImported{};
		size_t assetsToImport{};

		std::mutex mutex;
		
		std::string curFileLabel;

		bool doImportAsset(ImportAssetsDatabaseEntry& asset);

		Vector<Path> loadFont(const ImportAssetsDatabaseEntry& asset, Path dstDir);
		Vector<Path> genericImporter(const ImportAssetsDatabaseEntry& asset, Path dstDir);
		ImportResult importAsset(const ImportAssetsDatabaseEntry& asset, const MetadataFetchCallback& metadataFetcher, const AssetImporter& importer, Path assetsPath, AssetCollector::ProgressReporter progressReporter = {});
	};
}

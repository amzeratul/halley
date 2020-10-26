#pragma once
#include "halley/tools/tasks/editor_task.h"
#include "halley/file/path.h"
#include "import_assets_database.h"
#include <vector>
#include <set>

#include "asset_collector.h"

namespace Halley
{
	class Project;
	
	class ImportAssetsTask : public EditorTask
	{
	public:
		struct ImportResult {
			std::vector<AssetResource> out;
			std::vector<std::pair<Path, Bytes>> outFiles;
			std::vector<TimestampedPath> additionalInputs;
			bool success = false;
			String errorMsg;
		};
		using MetadataFetchCallback = std::function<std::optional<Metadata>(const Path&)>;
		
		ImportAssetsTask(String taskName, ImportAssetsDatabase& db, std::shared_ptr<AssetImporter> importer, Path assetsPath, Vector<ImportAssetsDatabaseEntry> files, std::vector<String> deletedAssets, Project& project, bool packAfter);

		static ImportResult importAsset(const ImportAssetsDatabaseEntry& asset, const MetadataFetchCallback& metadataFetcher, const AssetImporter& importer, Path assetsPath, AssetCollector::ProgressReporter progressReporter = {});

	protected:
		void run() override;

	private:
		ImportAssetsDatabase& db;
		std::shared_ptr<AssetImporter> importer;
		Path assetsPath;
		Project& project;
		const bool packAfter;

		Vector<ImportAssetsDatabaseEntry> files;
		std::vector<String> deletedAssets;
		std::set<String> outputAssets;
		
		std::atomic<int64_t> totalImportTime;
		std::atomic<size_t> assetsImported{};
		size_t assetsToImport{};

		std::mutex mutex;
		
		std::string curFileLabel;

		bool doImportAsset(ImportAssetsDatabaseEntry& asset);

		std::vector<Path> loadFont(const ImportAssetsDatabaseEntry& asset, Path dstDir);
		std::vector<Path> genericImporter(const ImportAssetsDatabaseEntry& asset, Path dstDir);
	};
}

#pragma once
#include "halley/file/path.h"
#include <map>
#include <mutex>
#include "halley/text/halleystring.h"
#include <cstdint>
#include "asset_importer.h"
#include "halley/core/resources/asset_database.h"

namespace Halley
{
	class Project;
	class Deserializer;
	class Serializer;
	
	class ImportAssetsDatabaseEntry
	{
	public:
		String assetId;
		Path srcDir;
		std::vector<TimestampedPath> inputFiles;
		std::vector<TimestampedPath> additionalInputFiles; // These were requested by the importer, rather than enumerated directly
		std::vector<AssetResource> outputFiles;
		ImportAssetType assetType = ImportAssetType::Undefined;

		ImportAssetsDatabaseEntry() {}

		ImportAssetsDatabaseEntry(String assetId, Path srcDir, Path inputFile, int64_t time)
			: assetId(assetId)
			, srcDir(srcDir)
			, inputFiles({ TimestampedPath(inputFile, time) })
		{}

		ImportAssetsDatabaseEntry(String assetId, Path srcDir)
			: assetId(assetId)
			, srcDir(srcDir)
		{}

		ImportAssetsDatabaseEntry(String assetId, Path srcDir, std::vector<TimestampedPath>&& inputFiles)
			: assetId(assetId)
			, srcDir(srcDir)
			, inputFiles(std::move(inputFiles))
		{}

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	class ImportAssetsDatabase
	{
		struct AssetEntry
		{
			ImportAssetsDatabaseEntry asset;

			bool present;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);
		};

	public:
		ImportAssetsDatabase(Path directory, Path dbFile, Path assetsDbFile, const String& Platform);

		void load();
		void save() const;

		bool needsImporting(const ImportAssetsDatabaseEntry& asset) const;
		void markAsImported(const ImportAssetsDatabaseEntry& asset);
		void markDeleted(const ImportAssetsDatabaseEntry& asset);
		void markFailed(const ImportAssetsDatabaseEntry& asset);

		void markAssetsAsStillPresent(const std::map<String, ImportAssetsDatabaseEntry>& assets);
		std::vector<ImportAssetsDatabaseEntry> getAllMissing() const;

		std::vector<AssetResource> getOutFiles(String assetId) const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		String platform;
		Path directory;
		Path dbFile;
		Path assetsDbFile;

		std::map<String, AssetEntry> assetsImported;
		std::map<String, AssetEntry> assetsFailed; // Ephemeral
		
		mutable std::mutex mutex;

		std::unique_ptr<AssetDatabase> makeAssetDatabase() const;
	};
}

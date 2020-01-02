#pragma once
#include "halley/file/path.h"
#include <map>
#include <mutex>
#include "halley/text/halleystring.h"
#include <cstdint>
#include <utility>
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

		ImportAssetsDatabaseEntry(String assetId, Path srcDir, const Path& inputFile, int64_t time)
			: assetId(std::move(assetId))
			, srcDir(std::move(srcDir))
			, inputFiles({ TimestampedPath(inputFile, time) })
		{}

		ImportAssetsDatabaseEntry(String assetId, Path srcDir)
			: assetId(std::move(assetId))
			, srcDir(std::move(srcDir))
		{}

		ImportAssetsDatabaseEntry(String assetId, Path srcDir, std::vector<TimestampedPath>&& inputFiles)
			: assetId(std::move(assetId))
			, srcDir(std::move(srcDir))
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

		class InputFileEntry
		{
		public:
			std::array<int64_t, 3> timestamp;
			Metadata metadata;
			Path basePath;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);
		};

	public:
		ImportAssetsDatabase(Path directory, Path dbFile, Path assetsDbFile, std::vector<String> platforms);

		void load();
		void save() const;
		std::unique_ptr<AssetDatabase> makeAssetDatabase(const String& platform) const;

		bool needToLoadInputMetadata(const Path& path, std::array<int64_t, 3> timestamps) const;
		void setInputFileMetadata(const Path& path, std::array<int64_t, 3> timestamps, const Metadata& data, Path basePath);
		Maybe<Metadata> getMetadata(const Path& path) const;
		Maybe<Path> getMetadataPath(AssetType type, const String& assetId) const;

		bool needsImporting(const ImportAssetsDatabaseEntry& asset) const;
		void markAsImported(const ImportAssetsDatabaseEntry& asset);
		void markDeleted(const ImportAssetsDatabaseEntry& asset);
		void markFailed(const ImportAssetsDatabaseEntry& asset);
		void markAssetsAsStillPresent(const std::map<String, ImportAssetsDatabaseEntry>& assets);
		std::vector<ImportAssetsDatabaseEntry> getAllMissing() const;

		std::vector<AssetResource> getOutFiles(String assetId) const;
		std::vector<String> getInputFiles() const;
		std::vector<std::pair<AssetType, String>> getAssetsFromFile(const Path& inputFile);

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		std::vector<String> platforms;
		Path directory;
		Path dbFile;
		Path assetsDbFile;

		std::map<String, AssetEntry> assetsImported;
		std::map<String, AssetEntry> assetsFailed; // Ephemeral
		std::map<String, InputFileEntry> inputFiles;
	
		mutable std::mutex mutex;
	};
}

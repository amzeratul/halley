#pragma once
#include "halley/file/filesystem.h"
#include <map>
#include <mutex>
#include "halley/text/halleystring.h"
#include <cstdint>
#include "asset_importer.h"

namespace Halley
{
	class Project;
	class Deserializer;
	class Serializer;
	
	class ImportAssetsDatabaseEntry
	{
	public:
		using InputFile = std::pair<Path, int64_t>;

		String assetId;
		Path srcDir;
		std::vector<InputFile> inputFiles;
		std::vector<Path> outputFiles;
		AssetType assetType = AssetType::UNDEFINED;

		ImportAssetsDatabaseEntry() {}

		ImportAssetsDatabaseEntry(String assetId, Path srcDir, Path inputFile, int64_t time)
			: assetId(assetId)
			, srcDir(srcDir)
			, inputFiles({ InputFile(inputFile, time) })
		{}

		ImportAssetsDatabaseEntry(String assetId, Path srcDir)
			: assetId(assetId)
			, srcDir(srcDir)
		{}

		ImportAssetsDatabaseEntry(String assetId, Path srcDir, std::vector<InputFile>&& inputFiles)
			: assetId(assetId)
			, srcDir(srcDir)
			, inputFiles(std::move(inputFiles))
		{}
	};

	class ImportAssetsDatabase
	{
		struct FileEntry
		{
			ImportAssetsDatabaseEntry asset;

			bool present;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);
		};

	public:
		ImportAssetsDatabase(Project& project, Path dbFile);

		void load();
		void save() const;

		bool needsImporting(const ImportAssetsDatabaseEntry& asset) const;
		void markAsImported(const ImportAssetsDatabaseEntry& asset);
		void markDeleted(const ImportAssetsDatabaseEntry& asset);

		void markAllAsMissing();
		void markAsPresent(const ImportAssetsDatabaseEntry& asset);
		std::vector<ImportAssetsDatabaseEntry> getAllMissing() const;

		std::vector<Path> getOutFiles(String assetId) const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		Project& project;
		Path dbFile;

		std::map<String, FileEntry> filesImported;
		
		mutable std::mutex mutex;
	};
}

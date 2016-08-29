#pragma once
#include "halley/file/filesystem.h"
#include <map>
#include <mutex>
#include "halley/text/halleystring.h"
#include <cstdint>

namespace Halley
{
	class Project;
	class Deserializer;
	class Serializer;

	class ImportAssetsDatabaseEntry
	{
	public:
		Path inputFile;
		Path srcDir;
		int64_t fileTime = 0;
		int64_t metaTime = 0;

		ImportAssetsDatabaseEntry() {}

		ImportAssetsDatabaseEntry(Path inputFile, Path srcDir, int64_t time, int64_t metaTime)
			: inputFile(inputFile)
			, srcDir(srcDir)
			, fileTime(time)
			, metaTime(time)
		{}
	};

	class ImportAssetsDatabase
	{
		struct FileEntry
		{
			ImportAssetsDatabaseEntry asset;
			std::vector<Path> outputFiles;

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
		void markDeleted(Path file);

		void markAllAsMissing();
		void markAsPresent(Path file);
		std::vector<Path> getAllMissing() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		Project& project;
		Path dbFile;

		std::map<Path, FileEntry> filesImported;
		
		mutable std::mutex mutex;
	};
}

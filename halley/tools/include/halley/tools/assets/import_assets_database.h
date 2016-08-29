#pragma once
#include "halley/file/filesystem.h"
#include <map>
#include <mutex>
#include "halley/text/halleystring.h"
#include <cstdint>

namespace Halley
{
	class Deserializer;
	class Serializer;

	class ImportAssetsDatabase
	{
		struct FileEntry
		{
			int64_t timestamp;
			bool present;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);
		};

	public:
		ImportAssetsDatabase(Path file);

		void load();
		void save() const;

		bool needsImporting(Path file, time_t timestamp) const;
		void markAsImported(Path file, time_t timestamp);
		void markDeleted(Path file);

		void markAllAsMissing();
		void markAsPresent(Path file);
		std::vector<Path> getAllMissing() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		Path file;
		std::map<String, FileEntry> filesImported;
		
		mutable std::mutex mutex;
	};
}

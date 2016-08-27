#pragma once
#include <boost/filesystem.hpp>
#include <map>
#include <thread>

namespace Halley
{
	using Path = boost::filesystem::path;

	class ImportAssetsDatabase
	{
		struct FileEntry
		{
			time_t timestamp;
			bool present;
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

	private:
		Path file;
		std::map<Path, FileEntry> filesImported;
		
		mutable std::mutex mutex;
	};
}

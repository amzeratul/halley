#include "import_assets_database.h"

using namespace Halley;

ImportAssetsDatabase::ImportAssetsDatabase(Path file)
	: file(file)
{}

void ImportAssetsDatabase::load()
{
	std::lock_guard<std::mutex> lock(mutex);
	// TODO
}

void ImportAssetsDatabase::save() const
{
	std::lock_guard<std::mutex> lock(mutex);
	// TODO
}

bool ImportAssetsDatabase::needsImporting(Path file, time_t timestamp) const
{
	std::lock_guard<std::mutex> lock(mutex);
	auto iter = filesImported.find(file);
	if (iter == filesImported.end()) {
		return true;
	} else {
		return timestamp > iter->second.timestamp;
	}
}

void ImportAssetsDatabase::markAsImported(Path file, time_t timestamp)
{
	std::lock_guard<std::mutex> lock(mutex);
	FileEntry entry;
	entry.timestamp = timestamp;
	entry.present = true;
	filesImported[file] = entry;
}

void ImportAssetsDatabase::markDeleted(Path file)
{
	std::lock_guard<std::mutex> lock(mutex);
	filesImported.erase(file);
}

void ImportAssetsDatabase::markAllAsMissing()
{
	std::lock_guard<std::mutex> lock(mutex);
	for (auto& e: filesImported) {
		e.second.present = false;
	}
}

void ImportAssetsDatabase::markAsPresent(Path file)
{
	std::lock_guard<std::mutex> lock(mutex);
	auto iter = filesImported.find(file);
	if (iter != filesImported.end()) {
		iter->second.present = true;
	}
}

std::vector<Path> ImportAssetsDatabase::getAllMissing() const
{
	std::lock_guard<std::mutex> lock(mutex);
	std::vector<Path> result;
	for (auto& e : filesImported) {
		if (!e.second.present) {
			result.push_back(e.first);
		}
	}
	return result;
}

#include "import_assets_database.h"

using namespace Halley;

ImportAssetsDatabase::ImportAssetsDatabase(Path file)
	: file(file)
{}

void ImportAssetsDatabase::load()
{
	// TODO
}

void ImportAssetsDatabase::save() const
{
	// TODO
}

bool ImportAssetsDatabase::needsImporting(Path file, time_t timestamp) const
{
	auto iter = filesImported.find(file);
	if (iter == filesImported.end()) {
		return true;
	} else {
		return timestamp > iter->second.timestamp;
	}
}

void ImportAssetsDatabase::markAsImported(Path file, time_t timestamp)
{
	FileEntry entry;
	entry.timestamp = timestamp;
	entry.present = true;
	filesImported[file] = entry;
}

void ImportAssetsDatabase::markDeleted(Path file)
{
	filesImported.erase(file);
}

void ImportAssetsDatabase::markAllAsMissing()
{
	for (auto& e: filesImported) {
		e.second.present = false;
	}
}

void ImportAssetsDatabase::markAsPresent(Path file)
{
	auto iter = filesImported.find(file);
	if (iter != filesImported.end()) {
		iter->second.present = true;
	}
}

std::vector<Path> ImportAssetsDatabase::getAllMissing() const
{
	std::vector<Path> result;
	for (auto& e : filesImported) {
		if (!e.second.present) {
			result.push_back(e.first);
		}
	}
	return result;
}

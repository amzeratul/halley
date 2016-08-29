#include "halley/tools/assets/import_assets_database.h"
#include "halley/file/byte_serializer.h"
#include "halley/resources/resource_data.h"
#include "halley/file/filesystem.h"

using namespace Halley;

void ImportAssetsDatabase::FileEntry::serialize(Serializer& s) const
{
	s << timestamp;
}

void ImportAssetsDatabase::FileEntry::deserialize(Deserializer& s)
{
	s >> timestamp;
}

ImportAssetsDatabase::ImportAssetsDatabase(Path file)
	: file(file)
{
	load();
}

void ImportAssetsDatabase::load()
{
	std::lock_guard<std::mutex> lock(mutex);
	try {
		auto data = FileSystem::readFile(file);
		auto s = Deserializer(data);
		deserialize(s);
	} catch (...) {
		// No database found, just ignore it
	}
}

void ImportAssetsDatabase::save() const
{
	std::lock_guard<std::mutex> lock(mutex);
	FileSystem::writeFile(file, Serializer::toBytes(*this));
}

bool ImportAssetsDatabase::needsImporting(Path file, time_t timestamp) const
{
	std::lock_guard<std::mutex> lock(mutex);
	auto iter = filesImported.find(file.string());
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
	filesImported[file.string()] = entry;
}

void ImportAssetsDatabase::markDeleted(Path file)
{
	std::lock_guard<std::mutex> lock(mutex);
	filesImported.erase(file.string());
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
	auto iter = filesImported.find(file.string());
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
			result.push_back(e.first.cppStr());
		}
	}
	return result;
}

void ImportAssetsDatabase::serialize(Serializer& s) const
{
	s << filesImported;
}

void ImportAssetsDatabase::deserialize(Deserializer& s)
{
	s >> filesImported;
}

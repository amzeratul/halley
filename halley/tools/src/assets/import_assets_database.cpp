#include "halley/tools/assets/import_assets_database.h"
#include "halley/file_formats/serializer.h"
#include "halley/resources/resource_data.h"

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
		auto data = ResourceDataStatic::loadFromFileSystem(file.string());
		auto s = Deserializer(data->getSpan());
		deserialize(s);
	} catch (...) {
		// No database found, just ignore it
	}
}

void ImportAssetsDatabase::save() const
{
	std::lock_guard<std::mutex> lock(mutex);
	
	auto bytes = Serializer::toBytes(*this);
	std::ofstream fp(file.string(), std::ios::binary | std::ios::out);
	fp.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	fp.close();
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

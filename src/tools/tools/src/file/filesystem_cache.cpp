#include "halley/tools/file/filesystem_cache.h"
#include "halley/tools/file/filesystem.h"

using namespace Halley;

void FileSystemCache::writeFile(const Path& path, Bytes data)
{
	FileSystem::writeFile(path, data);

	if (shouldCache(path, data.size())) {
		auto lock = std::unique_lock<std::mutex>(mutex);
		const auto key = path.getString();
		cache[key] = std::move(data);
	}
}

const Bytes& FileSystemCache::readFile(const Path& path)
{
	const auto key = path.getString();
	{
		auto lock = std::unique_lock<std::mutex>(mutex);
		const auto iter = cache.find(key);
		if (iter != cache.end()) {
			return iter->second;
		}
	}

	auto bytes = FileSystem::readFile(path);

	auto lock = std::unique_lock<std::mutex>(mutex);
	if (shouldCache(path, bytes.size())) {
		cache[key] = std::move(bytes);
		return cache.at(key);
	} else {
		static thread_local Bytes temp;
		temp = std::move(bytes);
		return temp;
	}
}

void FileSystemCache::remove(const Path& path)
{
	const auto key = path.getString();
	{
		auto lock = std::unique_lock<std::mutex>(mutex);
		const auto iter = cache.find(key);
		if (iter != cache.end()) {
			cache.erase(iter);
		}
	}

	FileSystem::remove(path);
}

bool FileSystemCache::hasCached(const Path& path) const
{
	auto lock = std::unique_lock<std::mutex>(mutex);
	const auto key = path.getString();
	return cache.contains(key);
}

bool FileSystemCache::shouldCache(const Path& path, size_t size)
{
	return size < 2048;
}

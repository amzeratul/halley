#include "halley/tools/file/filesystem_cache.h"

#include <filesystem>

#include "halley/support/logger.h"
#include "halley/tools/file/filesystem.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

void FileSystemCache::writeFile(const Path& path, Bytes data)
{
	FileSystem::writeFile(path, data);

	auto lock = std::unique_lock<std::mutex>(fileDataMutex);
	const auto key = path.getString();
	if (shouldCache(path, data.size())) {
		fileDataCache[key] = std::move(data);
	} else {
		fileDataCache.erase(key);
	}
}

const Bytes& FileSystemCache::readFile(const Path& path)
{
	const auto key = path.getString();
	{
		auto lock = std::unique_lock<std::mutex>(fileDataMutex);
		const auto iter = fileDataCache.find(key);
		if (iter != fileDataCache.end()) {
			return iter->second;
		}
	}

	auto bytes = FileSystem::readFile(path);

	auto lock = std::unique_lock<std::mutex>(fileDataMutex);
	if (shouldCache(path, bytes.size())) {
		fileDataCache[key] = std::move(bytes);
		return fileDataCache.at(key);
	} else {
		fileDataCache.erase(key);
		static thread_local Bytes temp;
		temp = std::move(bytes);
		return temp;
	}
}

void FileSystemCache::remove(const Path& path)
{
	const auto key = path.getString();
	{
		auto lock = std::unique_lock<std::mutex>(fileDataMutex);
		const auto iter = fileDataCache.find(key);
		if (iter != fileDataCache.end()) {
			fileDataCache.erase(iter);
		}
	}

	FileSystem::remove(path);
}

bool FileSystemCache::hasCached(const Path& path) const
{
	auto lock = std::unique_lock<std::mutex>(fileDataMutex);
	const auto key = path.getString();
	return fileDataCache.contains(key);
}

bool FileSystemCache::shouldCache(const Path& path, size_t size)
{
	return size < 2048;
}

Vector<Path> FileSystemCache::enumerateDirectory(const Path& path)
{
	auto lock = std::unique_lock<std::mutex>(fileTreeMutex);
	Vector<Path> result;
	const auto root = path.isDirectory() ? path : path / ".";
	doEnumerate(root, root, result);
	return result;
}

void FileSystemCache::doEnumerate(const Path& root, const Path& path, Vector<Path>& dst)
{
	const auto& dir = getDirectory(path);
	for (const auto& file: dir.files) {
		dst.push_back((path / file.name).makeRelativeTo(root));
	}
	for (const auto& dirName: dir.dirs) {
		doEnumerate(root, path / dirName / ".", dst);
	}
}

bool FileSystemCache::exists(const Path& path)
{
	auto lock = std::unique_lock<std::mutex>(fileTreeMutex);
	const auto& dir = getDirectory(path);
	const auto key = path.getFilename().string();
	return std_ex::contains_if(dir.files, [&](const FileEntry& f) { return f.name == key; });
}

int64_t FileSystemCache::getLastWriteTime(const Path& path)
{
	auto lock = std::unique_lock<std::mutex>(fileTreeMutex);
	const auto& dir = getDirectory(path);
	const auto key = path.getFilename().string();
	for (auto& f: dir.files) {
		if (f.name == key) {
			return f.lastWriteTime;
		}
	}
	return 0;
}

void FileSystemCache::trackDirectory(const Path& path)
{
	auto lock = std::unique_lock<std::mutex>(fileTreeMutex);
	if (!std_ex::contains(trackedDirs, path)) {
		trackedDirs.push_back(path);
		readDirFromFilesystem(path);
	}
}

FileSystemCache::DirEntry& FileSystemCache::getDirectory(const Path& path)
{
	if (auto* dir = tryGetDirectory(path)) {
		return *dir;
	}

	Logger::logError("FileSystemCache error: path \"" + path.getString(false) + "\" is not tracked by cache.");
	return emptyDir;
}

FileSystemCache::DirEntry* FileSystemCache::tryGetDirectory(const Path& path)
{
	const auto& dirPath = path.isDirectory() ? path : path.parentPath();
	const auto& key = dirPath.getString(false);
	const auto iter = dirs.find(key);
	if (iter != dirs.end()) {
		return &iter->second;
	}

	// Not found, create if it's in a tracked dir
	for (const auto& dir: trackedDirs) {
		if (dir.isPrefixOf(path)) {
			if (dir != path) {
				if (auto* parent = tryGetDirectory(dirPath.parentPath())) {
					parent->addDir(dirPath.getDirName().getString());
				}
			}
			return &dirs[key];
		}
	}

	return nullptr;
}

void FileSystemCache::readDirFromFilesystem(const Path& rootDir)
{
	if (!FileSystem::exists(rootDir)) {
		return;
	}

	const auto& key = rootDir.getString(false);
	auto& dir = dirs[key];
	const auto nativeRootDir = std::filesystem::path(rootDir.getNativeString().cppStr());

	Vector<Path> toRecurse;

	for (std::filesystem::directory_iterator iter(nativeRootDir); iter != std::filesystem::directory_iterator(); ++iter) {
		const auto fullPath = iter->path();

		if (std::filesystem::is_regular_file(fullPath)) {
			dir.addFile(Path(fullPath.string()));
		} else if (std::filesystem::is_directory(fullPath)) {
			const auto relPath = fullPath.lexically_relative(nativeRootDir);
			if (dir.addDir(relPath.string())) {
				// Defer recursion until loop is over, otherwise `dirs` can be modified and `dir` will be invalidated
				toRecurse.push_back(Path(fullPath.string()));
			}
		}
	}

	for (const auto& p: toRecurse) {
		readDirFromFilesystem(p);
	}
}

void FileSystemCache::DirEntry::addFile(const Path& fullPath)
{
	auto name = fullPath.getFilename().getString();
	if (!std_ex::contains_if(files, [&](const FileEntry& e) { return e.name == name; })) {
		files.push_back(FileEntry{ std::move(name), FileSystem::getLastWriteTime(fullPath) });
	}
}

void FileSystemCache::DirEntry::updateFile(const Path& fullPath)
{
	const auto name = fullPath.getFilename().getString();
	for (auto& file: files) {
		if (file.name == name) {
			file.lastWriteTime = FileSystem::getLastWriteTime(fullPath);
		}
	}
}

void FileSystemCache::DirEntry::removeFile(const Path& fullPath)
{
	const auto name = fullPath.getFilename().getString();
	std_ex::erase_if(files, [&](const FileEntry& e) { return e.name == name; });
}

bool FileSystemCache::DirEntry::addDir(const String& name)
{
	if (!std_ex::contains(dirs, name)) {
		dirs.push_back(name);
		return true;
	}
	return false;
}

void FileSystemCache::notifyChanges(gsl::span<const DirectoryMonitor::Event> events)
{
	for (const auto& event: events) {
		const auto filePath = Path(event.name);
		if (event.type == DirectoryMonitor::ChangeType::FileAdded) {
			getDirectory(event.name).addFile(filePath);
		} else if (event.type == DirectoryMonitor::ChangeType::FileModified) {
			getDirectory(event.name).updateFile(filePath);
		} else if (event.type == DirectoryMonitor::ChangeType::FileRemoved) {
			getDirectory(event.name).removeFile(filePath);
		} else if (event.type == DirectoryMonitor::ChangeType::FileRenamed) {
			const auto oldFilePath = Path(event.oldName);
			getDirectory(event.name).addFile(filePath);
			getDirectory(event.oldName).removeFile(oldFilePath);
		}
	}
}

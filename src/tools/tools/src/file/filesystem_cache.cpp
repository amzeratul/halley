#include "halley/tools/file/filesystem_cache.h"

#include <filesystem>

#include "halley/support/logger.h"
#include "halley/tools/file/filesystem.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

void FileSystemCache::writeFile(const Path& path, gsl::span<const gsl::byte> data)
{
	const auto key = path.getString();
	if (matchesCache(key, data)) {
		// No change, nothing to do here
		return;
	}

	FileSystem::writeFile(path, data);

	auto lock = std::unique_lock<std::mutex>(fileDataMutex);
	if (shouldCache(path, data.size())) {
		auto& result = fileDataCache[key];
		result.resize(data.size());
		memcpy(result.data(), data.data(), data.size());
	} else {
		fileDataCache.erase(key);
	}
}

void FileSystemCache::writeFile(const Path& path, Bytes data)
{
	const auto key = path.getString();
	if (matchesCache(key, gsl::as_bytes(data.span()))) {
		// No change, nothing to do here
		return;
	}

	FileSystem::writeFile(path, data);

	auto lock = std::unique_lock<std::mutex>(fileDataMutex);
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

bool FileSystemCache::shouldCache(const Path& path, size_t size) const
{
	return size < 2048;
}

bool FileSystemCache::matchesCache(const String& key, gsl::span<const gsl::byte> data) const
{
	auto lock = std::unique_lock<std::mutex>(fileDataMutex);
	if (const auto iter = fileDataCache.find(key); iter != fileDataCache.end()) {
		if (gsl::as_bytes(iter->second.span()) == data) {
			return true;
		}
	}
	return false;
}

Vector<Path> FileSystemCache::enumerateDirectory(const Path& path, bool includeDirs, bool recursive)
{
	auto lock = std::unique_lock<std::mutex>(fileTreeMutex);
	Vector<Path> result;
	const auto root = path.isDirectory() ? path : path / ".";
	doEnumerate(root, root, result, includeDirs, recursive);
	return result;
}

void FileSystemCache::doEnumerate(const Path& root, const Path& path, Vector<Path>& dst, bool includeDirs, bool recursive)
{
	const auto& dir = getDirectory(path);
	for (const auto& fileName: dir.filenames) {
		dst.push_back((path / fileName).makeRelativeTo(root));
	}

	for (const auto& dirName: dir.dirs) {
		const auto dirPath = path / dirName / ".";
		if (includeDirs) {
			dst.push_back(dirPath.makeRelativeTo(root));
		}
		if (recursive) {
			doEnumerate(root, dirPath, dst, includeDirs, recursive);
		}
	}
}

bool FileSystemCache::exists(const Path& path)
{
	auto lock = std::unique_lock<std::mutex>(fileTreeMutex);
	const auto& dir = getDirectory(path);
	return dir.files.contains(path.getFilenameStr());
}

int64_t FileSystemCache::getLastWriteTime(const Path& path)
{
	auto lock = std::unique_lock<std::mutex>(fileTreeMutex);
	const auto& dir = getDirectory(path);
	const auto key = path.getFilenameStr();
	const auto iter = dir.files.find(key);
	if (iter != dir.files.end()) {
		return iter->second.lastWriteTime;
	}
	return 0;
}

void FileSystemCache::trackDirectory(const Path& path)
{
	auto lock = std::unique_lock<std::mutex>(fileTreeMutex);
	const auto dirPath = path.isDirectory() ? path : path / ".";
	if (!std_ex::contains(trackedDirs, dirPath)) {
		trackedDirs.push_back(dirPath);
		readDirFromFilesystem(dirPath);
	}
}

FileSystemCache::DirEntry& FileSystemCache::getDirectory(const Path& path)
{
	if (auto* dir = tryGetDirectory(path)) {
		return *dir;
	}

	Logger::logError("FileSystemCache error: path \"" + path.getString() + "\" is not tracked by cache.");
	return emptyDir;
}

FileSystemCache::DirEntry* FileSystemCache::tryGetDirectory(const Path& path)
{
	const auto& dirPath = path.isDirectory() ? path : path.parentPath();
	assert(dirPath.isDirectory());
	if (dirPath == lastDirCache.first) {
		return lastDirCache.second;
	}

	const auto iter = dirs.find(dirPath);
	if (iter != dirs.end()) {
		lastDirCache = std::pair(dirPath, &iter->second);
		return &iter->second;
	}

	// Not found, create if it's in a tracked dir
	for (const auto& dir: trackedDirs) {
		if (dir.isPrefixOf(path)) {
			if (dir != path) {
				if (auto* parent = tryGetDirectory(dirPath.parentPath())) {
					parent->addDir(dirPath.getDirNameStr());
				}
			}
			lastDirCache = {};
			return &dirs[dirPath];
		}
	}

	return nullptr;
}

void FileSystemCache::readDirFromFilesystem(const Path& rootDir)
{
	if (!FileSystem::exists(rootDir)) {
		return;
	}

	lastDirCache = {};
	auto& dir = dirs[rootDir.isDirectory() ? rootDir : (rootDir / ".")];
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
	const auto name = fullPath.getFilenameStr();
	const auto iter = files.find(name);
	if (iter != files.end()) {
		iter->second.lastWriteTime = FileSystem::getLastWriteTime(fullPath);
	} else {
		files[name] = FileEntry{ FileSystem::getLastWriteTime(fullPath) };
		filenames.push_back(name);
	}
}

void FileSystemCache::DirEntry::updateFile(const Path& fullPath)
{
	files[fullPath.getFilenameStr()] = (FileEntry{ FileSystem::getLastWriteTime(fullPath) });
}

void FileSystemCache::DirEntry::removeFile(const Path& fullPath)
{
	const auto name = fullPath.getFilenameStr();
	files.erase(name);
	std_ex::erase(filenames, name);
}

bool FileSystemCache::DirEntry::addDir(const String& name)
{
	if (!std_ex::contains(dirs, name)) {
		dirs.push_back(name);
		return true;
	}
	return false;
}

void FileSystemCache::DirEntry::removeDir(const String& name)
{
	std_ex::erase(dirs, name);
}

void FileSystemCache::notifyChanges(gsl::span<const DirectoryMonitor::Event> events)
{
	for (const auto& event: events) {
		const auto filePath = Path(event.name);
		if (event.isDir) {
			const auto& name = filePath.getFilename().getString(false);
			auto parentDir = filePath.parentPath();
			if (event.type == DirectoryMonitor::ChangeType::FileAdded) {
				getDirectory(parentDir).addDir(name);
				readDirFromFilesystem(filePath);
			} else if (event.type == DirectoryMonitor::ChangeType::FileModified) {
				// Nothing to do here
			} else if (event.type == DirectoryMonitor::ChangeType::FileRemoved) {
				getDirectory(parentDir).removeDir(name);
				dirs.erase(filePath);
			} else if (event.type == DirectoryMonitor::ChangeType::FileRenamed) {
				const auto oldFilePath = Path(event.oldName);
				getDirectory(parentDir).addDir(name);
				getDirectory(Path(event.oldName).parentPath()).removeDir(oldFilePath.getFilename().getString(false));
				dirs.erase(Path(event.oldName));
				readDirFromFilesystem(filePath);
			}
		} else {
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
}

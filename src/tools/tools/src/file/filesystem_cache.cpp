#include "halley/tools/file/filesystem_cache.h"

#include <filesystem>

#include "halley/support/logger.h"
#include "halley/tools/file/filesystem.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

void FileSystemCache::writeFile(const Path& path, gsl::span<const std::byte> data)
{
	const auto key = path.getString();
	if (matchesCache(key, data)) {
		// No change, nothing to do here
		return;
	}

	FileSystem::writeFile(path, data);

	auto lock = UniqueLock(fileDataMutex);
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

	auto lock = UniqueLock(fileDataMutex);
	if (shouldCache(path, data.size())) {
		fileDataCache[key] = std::move(data);
	} else {
		fileDataCache.erase(key);
	}
}

void FileSystemCache::writeFile(const Path& path, const String& data)
{
	writeFile(path, gsl::as_bytes(gsl::span<const char>(data.c_str(), data.length())));
}

gsl::span<const std::byte> FileSystemCache::readFile(const Path& path)
{
	const auto key = path.getString();
	{
		auto lock = SharedLock(fileDataMutex);
		const auto iter = fileDataCache.find(key);
		if (iter != fileDataCache.end()) {
			return iter->second.const_byte_span();
		}
	}

	auto bytes = FileSystem::readFile(path);

	auto lock = UniqueLock(fileDataMutex);
	if (shouldCache(path, bytes.size())) {
		fileDataCache[key] = std::move(bytes);
		return fileDataCache.at(key).const_byte_span();
	} else {
		fileDataCache.erase(key);
		static thread_local Bytes temp;
		temp = std::move(bytes);
		return temp.const_byte_span();
	}
}

Bytes FileSystemCache::readFileCopy(const Path& path)
{
	const auto key = path.getString();
	{
		auto lock = SharedLock(fileDataMutex);
		const auto iter = fileDataCache.find(key);
		if (iter != fileDataCache.end()) {
			return iter->second;
		}
	}

	auto bytes = FileSystem::readFile(path);

	auto lock = UniqueLock(fileDataMutex);
	if (shouldCache(path, bytes.size())) {
		fileDataCache[key] = bytes;
	} else {
		fileDataCache.erase(key);
	}

	return bytes;
}

bool FileSystemCache::remove(const Path& path)
{
	bool modified = false;

	const auto key = path.getString();
	{
		auto lock = UniqueLock(fileDataMutex);
		const auto iter = fileDataCache.find(key);
		if (iter != fileDataCache.end()) {
			fileDataCache.erase(iter);
			modified = true;
		}
	}

	return FileSystem::remove(path) || modified;
}

bool FileSystemCache::hasCached(const Path& path) const
{
	auto lock = SharedLock(fileDataMutex);
	const auto key = path.getString();
	return fileDataCache.contains(key);
}

bool FileSystemCache::hasCached(std::string_view path) const
{
	auto lock = SharedLock(fileDataMutex);
	return fileDataCache.contains(path);
}

bool FileSystemCache::shouldCache(const Path& path, size_t size) const
{
	return size < 2048;
}

bool FileSystemCache::matchesCache(const String& key, gsl::span<const std::byte> data) const
{
	auto lock = UniqueLock(fileDataMutex);
	if (const auto iter = fileDataCache.find(key); iter != fileDataCache.end()) {
		if (gsl::as_bytes(iter->second.span()) == data) {
			return true;
		}
	}
	return false;
}

Vector<Path> FileSystemCache::enumerateDirectory(const Path& path, bool includeDirs, bool recursive)
{
	auto lock = UniqueLock(fileTreeMutex);
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

Vector<FileSystemCache::DirectoryListing> FileSystemCache::enumerateDirectoryListings(const Path& path)
{
	auto lock = UniqueLock(fileTreeMutex);
	Vector<DirectoryListing> result;
	const auto root = path.isDirectory() ? path : path / ".";
	doEnumerateListings(root, root, result);
	return result;
}

void FileSystemCache::doEnumerateListings(const Path& root, const Path& path, Vector<DirectoryListing>& dst)
{
	const auto& dir = getDirectory(path);

	DirectoryListing listing;
	listing.dir = path.makeRelativeTo(root);
	listing.files.reserve(dir.filenames.size());
	for (const auto& fileName: dir.filenames) {
		const auto iter = dir.files.find(getCaseCorrectedPath(fileName));
		listing.files.emplace_back(fileName, iter != dir.files.end() ? iter->second.lastWriteTime : 0);
	}
	dst.push_back(std::move(listing));

	for (const auto& dirName: dir.dirs) {
		doEnumerateListings(root, path / dirName / ".", dst);
	}
}

bool FileSystemCache::exists(const Path& path)
{
	String buffer;
	const auto& name = getCaseCorrectedPath(path.getFilenameStrView(), buffer);
	auto lock = UniqueLock(fileTreeMutex);
	const auto& dir = getDirectory(path);
	return dir.files.contains(name);
}

int64_t FileSystemCache::getLastWriteTime(const Path& path)
{
	return tryGetLastWriteTime(path).value_or(0);
}

std::optional<int64_t> FileSystemCache::tryGetLastWriteTime(const Path& path)
{
	const auto key = getCaseCorrectedPath(path.getFilename());
	auto lock = UniqueLock(fileTreeMutex);
	const auto& dir = getDirectory(path);
	const auto iter = dir.files.find(key);
	if (iter != dir.files.end()) {
		return iter->second.lastWriteTime;
	}
	return std::nullopt;
}

void FileSystemCache::trackDirectory(const Path& path)
{
	auto lock = UniqueLock(fileTreeMutex);
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
	HalleyAssertDev(dirPath.isDirectory());
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
					parent->addDir(dirPath.getDirName());
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

		std::error_code ec;
		if (std::filesystem::is_regular_file(fullPath, ec)) {
			dir.addFile(Path(fullPath.string()));
		} else if (std::filesystem::is_directory(fullPath, ec)) {
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

Path FileSystemCache::getCaseCorrectedPath(Path p)
{
	if (!Path::isCaseSensitive()) {
		p.makeLowerCase();
	}
	return p;
}

String FileSystemCache::getCaseCorrectedPath(String p)
{
	if (!Path::isCaseSensitive()) {
		p.asciiMakeLower();
	}
	return p;
}

std::string_view FileSystemCache::getCaseCorrectedPath(std::string_view p, String& buffer)
{
	if (!Path::isCaseSensitive() && !String::isLowerCase(p)) {
		buffer = p;
		buffer.asciiMakeLower();
		return buffer;
	}
	return p;
}

void FileSystemCache::DirEntry::addFile(const Path& fullPath)
{
	const auto name = fullPath.getFilename();
	const auto iter = files.find(getCaseCorrectedPath(name));
	if (iter != files.end()) {
		iter->second.lastWriteTime = FileSystem::getLastWriteTime(fullPath);
	} else {
		files[getCaseCorrectedPath(name)] = FileEntry{ FileSystem::getLastWriteTime(fullPath) };
		filenames.push_back(name);
	}
}

void FileSystemCache::DirEntry::updateFile(const Path& fullPath)
{
	files[getCaseCorrectedPath(fullPath.getFilename())] = (FileEntry{ FileSystem::getLastWriteTime(fullPath) });
}

void FileSystemCache::DirEntry::removeFile(const Path& fullPath)
{
	const auto name = fullPath.getFilename();
	files.erase(getCaseCorrectedPath(name));
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
			const auto& name = filePath.getFilename();
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
				getDirectory(Path(event.oldName).parentPath()).removeDir(oldFilePath.getFilename());
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

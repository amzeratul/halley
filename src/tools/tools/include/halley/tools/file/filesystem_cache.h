#pragma once

#include "halley/file/path.h"
#include "halley/data_structures/hash_map.h"
#include "halley/file/directory_monitor.h"
#include "halley/game/scene_editor_interface.h"

namespace Halley {
    class FileSystemCache: public IFileSystemCache {
    public:
        void writeFile(const Path& path, gsl::span<const std::byte> data);
        void writeFile(const Path& path, Bytes data);
        void writeFile(const Path& path, const String& data);
		gsl::span<const std::byte> readFile(const Path& path) override;
		Bytes readFileCopy(const Path& path) override;
        bool remove(const Path& path);
        bool hasCached(const Path& path) const;
        bool hasCached(std::string_view path) const;

    	Vector<Path> enumerateDirectory(const Path& path, bool includeDirs = false, bool recursive = true);
		bool exists(const Path& path);
		int64_t getLastWriteTime(const Path& path);
		std::optional<int64_t> tryGetLastWriteTime(const Path& path);

		struct DirectoryListing {
			Path dir; // Relative to the enumerated root
			Vector<std::pair<String, int64_t>> files; // Filename -> last write time, in directory order
		};
		Vector<DirectoryListing> enumerateDirectoryListings(const Path& path);

        void trackDirectory(const Path& path);
		void notifyChanges(gsl::span<const DirectoryMonitor::Event> events);

        static String getCaseCorrectedPath(String p);

    private:
        struct FileEntry {
            int64_t lastWriteTime = 0;
        };

        struct DirEntry {
            HashMap<String, FileEntry> files;
            Vector<String> filenames; // Kept separately so it stays in order
            Vector<String> dirs;

            void addFile(const Path& fullPath);
            void updateFile(const Path& fullPath);
            void removeFile(const Path& fullPath);
            bool addDir(const String& name);
            void removeDir(const String& name);
        };

        mutable SharedMutex fileDataMutex;
        mutable Mutex fileTreeMutex;

        HashMap<String, Bytes> fileDataCache;
        HashMap<Path, DirEntry> dirs;
        DirEntry emptyDir;
        Vector<Path> trackedDirs;

        mutable std::pair<Path, DirEntry*> lastDirCache;

        bool shouldCache(const Path& path, size_t size) const;
        bool matchesCache(const String& key, gsl::span<const std::byte> data) const;

        void doEnumerate(const Path& root, const Path& path, Vector<Path>& dst, bool includeDirs, bool recursive);
        void doEnumerateListings(const Path& root, const Path& path, Vector<DirectoryListing>& dst);

        DirEntry& getDirectory(const Path& path);
        DirEntry* tryGetDirectory(const Path& path);
        void readDirFromFilesystem(const Path& rootDir);

        static Path getCaseCorrectedPath(Path p);
    };
}

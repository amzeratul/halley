#pragma once

#include "halley/file/path.h"
#include "halley/data_structures/hash_map.h"
#include "halley/file/directory_monitor.h"

namespace Halley {
    class FileSystemCache {
    public:
        void writeFile(const Path& path, Bytes data);
		const Bytes& readFile(const Path& path);
        void remove(const Path& path);
        bool hasCached(const Path& path) const;

    	Vector<Path> enumerateDirectory(const Path& path);
		bool exists(const Path& path);
		int64_t getLastWriteTime(const Path& path);

        void trackDirectory(const Path& path);
		void notifyChanges(gsl::span<const DirectoryMonitor::Event> events);

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
        };

        mutable std::mutex fileDataMutex;
        mutable std::mutex fileTreeMutex;

        HashMap<String, Bytes> fileDataCache;
        HashMap<Path, DirEntry> dirs;
        DirEntry emptyDir;
        Vector<Path> trackedDirs;

        mutable std::pair<Path, DirEntry*> lastDirCache;

        bool shouldCache(const Path& path, size_t size);

        void doEnumerate(const Path& root, const Path& path, Vector<Path>& dst);

        DirEntry& getDirectory(const Path& path);
        DirEntry* tryGetDirectory(const Path& path);
        void readDirFromFilesystem(const Path& rootDir);
    };
}

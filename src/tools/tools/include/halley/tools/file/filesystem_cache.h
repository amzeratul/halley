#pragma once

#include "halley/file/path.h"
#include "halley/data_structures/hash_map.h"

namespace Halley {
    class FileSystemCache {
    public:
        void writeFile(const Path& path, Bytes data);
		const Bytes& readFile(const Path& path);
        void remove(const Path& path);
        bool hasCached(const Path& path) const;

    private:
        HashMap<String, Bytes> cache;
        mutable std::mutex mutex;

        bool shouldCache(const Path& path, size_t size);
    };
}

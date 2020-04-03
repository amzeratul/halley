#pragma once

#include <gsl/gsl>
#include "halley/utils/utils.h"
#include <vector>
#include "halley/file/path.h"
#include "halley/data_structures/maybe.h"

namespace Halley {
	class String;
	class Path;

	class FileSystem
	{
	public:
		static bool exists(const Path& p);
		static bool createDir(const Path& p);
		static bool createParentDir(const Path& p);

		static int64_t getLastWriteTime(const Path& p);
		static bool isFile(const Path& p);
		static bool isDirectory(const Path& p);

		static void copyFile(const Path& src, const Path& dst);
		static bool remove(const Path& path);

		static void writeFile(const Path& path, gsl::span<const gsl::byte> data);
		static void writeFile(const Path& path, const Bytes& data);
		static Bytes readFile(const Path& path);

		static std::vector<Path> enumerateDirectory(const Path& path);
		
		static Path getRelative(const Path& path, const Path& parentPath);
		static Path getAbsolute(const Path& path);

		static size_t fileSize(const Path& path);

		static Path getTemporaryPath();

		static int runCommand(const String& command);
	};

	class ScopedTemporaryFile final {
	public:
		ScopedTemporaryFile();
		ScopedTemporaryFile(const String& extension);
		~ScopedTemporaryFile();

		ScopedTemporaryFile(const ScopedTemporaryFile& other) = delete;
		ScopedTemporaryFile(ScopedTemporaryFile&& other);
		ScopedTemporaryFile& operator=(const ScopedTemporaryFile& other) = delete;
		ScopedTemporaryFile& operator=(ScopedTemporaryFile&& other);

		const Path& getPath() const;

	private:
		std::optional<Path> path;
	};
}


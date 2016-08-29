#pragma once

#include <gsl/gsl>
#include "halley/utils/utils.h"
#include <boost/filesystem/path.hpp>

namespace Halley
{
	using Path = boost::filesystem::path;

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
	};
}

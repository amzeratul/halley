#include "halley/file/filesystem.h"
#include <boost/filesystem.hpp>
#include "halley/support/exception.h"

using namespace Halley;
using namespace filesystem;

bool FileSystem::exists(const Path& p)
{
	return filesystem::exists(p.getNative());
}

bool FileSystem::createDir(const Path& p)
{
	if (!exists(p)) {
		try {
			return create_directories(p.getNative());
		} catch (...) {
			return false;
		}
	}
	return false;
}

bool FileSystem::createParentDir(const Path& p)
{
	return createDir(p.getNative().parent_path());
}

int64_t FileSystem::getLastWriteTime(const Path& p)
{
	return last_write_time(p.getNative());
}

bool FileSystem::isFile(const Path& p)
{
	return is_regular_file(p.getNative());
}

bool FileSystem::isDirectory(const Path& p)
{
	return is_directory(p.getNative());
}

void FileSystem::copyFile(const Path& src, const Path& dst)
{
	createParentDir(dst);
	copy_file(src.getNative(), dst.getNative(), filesystem::copy_option::overwrite_if_exists);
}

bool FileSystem::remove(const Path& path)
{
	return filesystem::remove(path.getNative());
}

void FileSystem::writeFile(const Path& path, gsl::span<const gsl::byte> data)
{
	createParentDir(path);
	std::ofstream fp(path.string(), std::ios::binary | std::ios::out);
	fp.write(reinterpret_cast<const char*>(data.data()), data.size());
	fp.close();
}

void FileSystem::writeFile(const Path& path, const Bytes& data)
{
	writeFile(path, as_bytes(gsl::span<const Byte>(data)));
}

Bytes FileSystem::readFile(const Path& path)
{
	Bytes result;

	std::ifstream fp(path.string(), std::ios::binary | std::ios::in);
	if (!fp.is_open()) {
		throw Exception("Unable to open file at " + path.string());
	}

	fp.seekg(0, std::ios::end);
	size_t size = fp.tellg();
	fp.seekg(0, std::ios::beg);
	result.resize(size);

	fp.read(reinterpret_cast<char*>(result.data()), size);
	fp.close();

	return result;
}

std::vector<Path> FileSystem::enumerateDirectory(const Path& dir)
{
	std::vector<Path> result;
	if (exists(dir)) {
		using RDI = filesystem::recursive_directory_iterator;
		RDI end;
		for (RDI i(dir.getNative()); i != end; ++i) {
			path fullPath = i->path();
			if (is_regular_file(fullPath.native())) {
				result.push_back(Path(fullPath.lexically_relative(dir.getNative())));
			}
		}
	}
	return result;
}

Path FileSystem::getRelative(const Path& path, const Path& parentPath)
{
	return relative(path.getNative(), parentPath.getNative());
}

Path FileSystem::getAbsolute(const Path& path)
{
	return path.getNative().lexically_normal();
}

size_t FileSystem::fileSize(const Path& path)
{
	return file_size(path.getNative());
}

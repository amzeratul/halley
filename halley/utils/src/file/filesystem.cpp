#include "halley/file/filesystem.h"
#include <boost/filesystem.hpp>

using namespace Halley;

bool FileSystem::exists(const Path& p)
{
	return boost::filesystem::exists(p);
}

bool FileSystem::createDir(const Path& p)
{
	return boost::filesystem::create_directories(p);
}

bool FileSystem::createParentDir(const Path& p)
{
	return boost::filesystem::create_directories(p.parent_path());
}

int64_t FileSystem::getLastWriteTime(const Path& p)
{
	return boost::filesystem::last_write_time(p);
}

bool FileSystem::isFile(const Path& p)
{
	return boost::filesystem::is_regular_file(p);
}

bool FileSystem::isDirectory(const Path& p)
{
	return boost::filesystem::is_directory(p);
}

void FileSystem::copyFile(const Path& src, const Path& dst)
{
	createParentDir(dst);
	boost::filesystem::copy_file(src, dst, boost::filesystem::copy_option::overwrite_if_exists);
}

bool FileSystem::remove(const Path& path)
{
	return boost::filesystem::remove(path);
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
	writeFile(path, gsl::as_bytes(gsl::span<const Halley::Byte>(data)));
}

Bytes FileSystem::readFile(const Path& path)
{
	Bytes result;

	std::ifstream fp(path.string(), std::ios::binary | std::ios::in);
	fp.seekg(0, std::ios::end);
	size_t size = fp.tellg();
	fp.seekg(0, std::ios::beg);
	result.resize(size);

	fp.read(reinterpret_cast<char*>(result.data()), size);
	fp.close();

	return result;
}

std::vector<Path> FileSystem::enumerateDirectory(const Path& path)
{
	std::vector<Path> result;
	if (exists(path)) {
		using RDI = boost::filesystem::recursive_directory_iterator;
		RDI end;
		for (RDI i(path); i != end; ++i) {
			Path fullPath = i->path();
			if (FileSystem::isFile(fullPath)) {
				result.push_back(fullPath.lexically_relative(path));
			}
		}
	}
	return result;
}

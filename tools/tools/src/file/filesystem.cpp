#include "halley/tools/file/filesystem.h"
#include <boost/filesystem.hpp>
#include <halley/file/path.h>
#include "halley/os/os.h"

using namespace Halley;
using namespace boost::filesystem;

static path getNative(const Path& p)
{
	return path(p.string());
}

bool FileSystem::exists(const Path& p)
{
	return ::exists(getNative(p));
}

bool FileSystem::createDir(const Path& p)
{
	if (!exists(p)) {
		try {
			return create_directories(getNative(p));
		} catch (...) {
			return false;
		}
	}
	return false;
}

bool FileSystem::createParentDir(const Path& p)
{
	return createDir(getNative(p).parent_path().string());
}

int64_t FileSystem::getLastWriteTime(const Path& p)
{
	return last_write_time(getNative(p));
}

bool FileSystem::isFile(const Path& p)
{
	return is_regular_file(getNative(p));
}

bool FileSystem::isDirectory(const Path& p)
{
	return is_directory(getNative(p));
}

void FileSystem::copyFile(const Path& src, const Path& dst)
{
	createParentDir(dst);
	copy_file(getNative(src), getNative(src), copy_option::overwrite_if_exists);
}

bool FileSystem::remove(const Path& path)
{
	boost::system::error_code ec;
	return boost::filesystem::remove_all(getNative(path), ec) > 0;
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
		return result;
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
		using RDI = recursive_directory_iterator;
		RDI end;
		for (RDI i(getNative(dir)); i != end; ++i) {
			path fullPath = i->path();
			if (is_regular_file(fullPath.native())) {
				result.push_back(Path(fullPath.lexically_relative(getNative(dir)).string()));
			}
		}
	}
	return result;
}

Path FileSystem::getRelative(const Path& path, const Path& parentPath)
{
	return relative(getNative(path), getNative(parentPath)).string();
}

Path FileSystem::getAbsolute(const Path& path)
{
	return getNative(path).lexically_normal().string();
}

size_t FileSystem::fileSize(const Path& path)
{
	return file_size(getNative(path));
}

Path FileSystem::getTemporaryPath() 
{
	return (temp_directory_path() / unique_path()).string();
}

int FileSystem::runCommand(const String& command)
{
	return OS::get().runCommand(command);
}

#include "halley/tools/file/filesystem.h"
#include <boost/filesystem.hpp>
#include <halley/file/path.h>
#include "halley/os/os.h"
#include "halley/maths/random.h"
#include <cstdio>

#ifdef _WIN32
#include <Windows.h>
#endif

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
	try {
		if (!exists(p)) {
			return create_directories(getNative(p));
		} else {
			return false;
		}
	} catch (...) {
		return false;
	}
}

bool FileSystem::createParentDir(const Path& p)
{
	return createDir(getNative(p).parent_path().string());
}

int64_t FileSystem::getLastWriteTime(const Path& p)
{
	boost::system::error_code ec;
	const auto result = last_write_time(getNative(p), ec);
	if (ec.failed()) {
		return 0;
	}
	return result;
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
	int nRemoved = boost::filesystem::remove_all(getNative(path), ec) > 0;
	return nRemoved > 0 && ec.value() == 0;
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

void FileSystem::writeFile(const Path& path, const String& data)
{
	writeFile(path, as_bytes(gsl::span<const char>(data.c_str(), data.length())));
}

Bytes FileSystem::readFile(const Path& path)
{
	Bytes result;

#if defined(_WIN32)
	const auto pathStr = path.toString().getUTF16();
	const auto handle = CreateFileW(pathStr.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	
	if (!handle) {
		return {};
	}

	// Get size
	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx(handle, &fileSize)) {
		CloseHandle(handle);
		return {};
	}
	if (fileSize.HighPart > 0) {
		CloseHandle(handle);
		return {};
	}

	// Read
	result.resize(fileSize.LowPart);
	DWORD toRead = fileSize.LowPart;
	DWORD totalRead = 0;
	while (toRead > 0) {
		DWORD nRead = 0;
		if (!ReadFile(handle, result.data() + totalRead, toRead, &nRead, nullptr)) {
			CloseHandle(handle);
			return {};
		}
		totalRead += nRead;
		toRead -= nRead;
	}

	// Clean up
	CloseHandle(handle);
	return result;
	
#else
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
#endif

	return result;
}

Vector<Path> FileSystem::enumerateDirectory(const Path& dir)
{
	Vector<Path> result;
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
	auto& rng = Random::getGlobal();
	std::array<char, 16> name;
	const std::array<char, 16> digits = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
	for (size_t i = 0; i < name.size(); ++i) {
		name[i] = digits[rng.getInt(0, 15)];
	}
	return Path(temp_directory_path().string()) / Path(String(name.data(), name.size()));
}

int FileSystem::runCommand(const String& command)
{
	return OS::get().runCommand(command);
}

ScopedTemporaryFile::ScopedTemporaryFile()
{
	path = FileSystem::getTemporaryPath();
}

ScopedTemporaryFile::ScopedTemporaryFile(const String& extension)
{
	path = FileSystem::getTemporaryPath().replaceExtension(extension);
}

ScopedTemporaryFile::~ScopedTemporaryFile()
{
	using namespace std::chrono_literals;

	if (path) {
		for (int i = 0; i < 5; ++i) {
			bool ok = FileSystem::remove(*path);
			if (ok) {
				break;
			}
			std::this_thread::sleep_for(20ms * (i + 1));
		}
	}
}

ScopedTemporaryFile::ScopedTemporaryFile(ScopedTemporaryFile&& other) noexcept
{
	path = std::move(other.path);
	other.path.reset();
}

ScopedTemporaryFile& ScopedTemporaryFile::operator=(ScopedTemporaryFile&& other) noexcept
{
	path = std::move(other.path);
	other.path.reset();
	return *this;
}

const Path& ScopedTemporaryFile::getPath() const
{
	if (path) {
		return *path;
	}
	throw Exception("Path not available", HalleyExceptions::File);
}

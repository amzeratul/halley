#include "halley/file/path.h"

#ifndef _LIBCPP_HAS_NO_FILESYSTEM_LIBRARY
#include <filesystem>
#endif

#include <sstream>
#include <fstream>

#ifdef _MSC_VER
	#include <sys/utime.h>
#else
	#include <sys/types.h>
	#include <utime.h>
#endif

#include "halley/os/os.h"
#include "halley/utils/hash.h"

using namespace Halley;

Path::Path()
{}

Path::Path(const char* name)
{
	setPath(name);
}

Path::Path(const std::string& name)
{
	setPath(name);
}

Path::Path(const String& name)
{
	setPath(name);
}

void Path::setPath(const String& value)
{
	String rawPath = value;
#ifdef _WIN32
	rawPath = rawPath.replaceAll("\\", "/");
#endif

	pathParts = rawPath.split('/');
	normalise();
}

Path::Path(Vector<String> parts, bool normaliseAfter)
	: pathParts(std::move(parts))
{
	if (normaliseAfter) {
		normalise();
	}
}

void Path::normalise()
{
	size_t writePos = 0;
	bool lastIsBack = false;

	auto write = [&] (const String& p)
	{
		pathParts.at(writePos++) = p;
		lastIsBack = false;
	};

	auto canInsertDot = [&] () -> bool
	{
		return writePos == 0 || (pathParts[writePos - 1] != "." && pathParts[writePos - 1] != "..");
	};

	int n = int(pathParts.size());
	for (int i = 0; i < n; ++i) {
		bool first = i == 0;
		bool last = i == n - 1;

		String current = pathParts[i]; // Important: don't make this a reference
		if (current == "") {
			if (first) {
				write(current);
			} else if (last) {
				write(".");
			}
		} else if (current == ".") {
			if (last && canInsertDot()) {
				write(current);
			}
		} else if (current == "..") {
			if (writePos > 0 && pathParts[writePos - 1] != ".." && pathParts[writePos - 1] != ".") {
				--writePos;
				lastIsBack = true;
			} else {
				write(current);
			}
		} else {
			write(current);
		}
	}
	if (lastIsBack && canInsertDot()) {
		write(".");
	}

	pathParts.resize(writePos);

#ifdef _WIN32
	if (!pathParts.empty()) {
		if (pathParts[0].size() == 2 && pathParts[0][1] == ':') {
			pathParts[0] = pathParts[0].asciiUpper();
		}
	}
#endif
}

Path& Path::operator=(const std::string& other)
{
	setPath(other);
	return *this;
}

Path& Path::operator=(const String& other)
{
	setPath(other);
	return *this;
}

Path Path::getFilename() const
{
	return pathParts.back();
}

const String& Path::getFilenameStr() const
{
	return pathParts.back();
}

Path Path::getDirName() const
{
	if (pathParts.back() == ".") {
		return pathParts[pathParts.size() - 2];
	}
	return "";
}

const String& Path::getDirNameStr() const
{
	if (pathParts.back() == ".") {
		return pathParts[pathParts.size() - 2];
	}
	return String::emptyString();
}

Path Path::getStem() const
{
	String filename = pathParts.back();
	if (filename == "." || filename == "..") {
		return filename;
	}
	size_t dotPos = filename.find_last_of('.');
	return filename.substr(0, dotPos);
}

String Path::getExtension() const
{
	if (pathParts.empty()) {
		return "";
	}
	String filename = pathParts.back();
	if (filename == "." || filename == "..") {
		return filename;
	}
	size_t dotPos = filename.find_last_of('.');
	return filename.substr(dotPos);
}

std::string Path::makeString(bool includeDot, char dirSeparator) const
{
	std::string result;

	// Measure size needed
	bool first = true;
	size_t size = 0;
	for (const auto& p: pathParts) {
		if (&p == &pathParts.back() && p == "." && !includeDot) {
			break;
		}
		if (first) {
			first = false;
		} else {
			// Separator
			size += 1;
		}
		size += p.size();
	}
	result.reserve(size);

	// Write string
	first = true;
	for (auto& p : pathParts) {
		if (&p == &pathParts.back() && p == "." && !includeDot) {
			break;
		}
		if (first) {
			first = false;
		} else {
			result += dirSeparator;
		}
		result += p;
	}
	return result;
}

std::string Path::string() const
{
	return makeString(true, '/');
}

String Path::getString(bool includeDot) const
{
	return makeString(includeDot, '/');
}

String Path::getNativeString(bool includeDot) const
{
#ifdef _WIN32
	constexpr char separator = '\\';
#else
	constexpr char separator = '/';
#endif

	return makeString(includeDot, separator);
}

String Path::toString() const
{
	return makeString(true, '/');
}

gsl::span<const String> Path::getParts() const
{
	return pathParts;
}

size_t Path::getNumberPaths() const
{
	return pathParts.size();
}

Path Path::dropFront(int numberFolders) const
{
	return Path(Vector<String>(pathParts.begin() + numberFolders, pathParts.end()), true);
}

Path Path::parentPath() const
{
	Path result = *this;
	if (isDirectory()) {
		result.pathParts.pop_back();
		if (!result.pathParts.empty()) {
			result.pathParts.pop_back();
		}
		result.pathParts.push_back(".");
	} else {
		if (!result.pathParts.empty()) {
			result.pathParts.back() = ".";
		}
	}
	return result;
}

Path Path::replaceExtension(String newExtension) const
{
	auto parts = pathParts;
	parts.back() = getStem().getString() + newExtension;
	return Path(parts, true);
}

Path Path::operator/(std::string_view other) const
{
	if (other != ".." && other.find('/') == std::string_view::npos && other.find('\\') == std::string_view::npos) {
		Path p = *this;
		if (!p.pathParts.empty() && p.pathParts.back() == ".") {
			p.pathParts.pop_back();
		}
		p.pathParts.push_back(other);
		return p;
	}
	return operator/(Path(other));
}

Path Path::operator/(const char* other) const
{
	return operator/(std::string_view(other));
}

Path Path::operator/(const String& other) const
{
	return operator/(std::string_view(other));
}

Path Path::operator/(const std::string& other) const
{
	return operator/(std::string_view(other));
}

Path Path::operator/(const Path& other) const 
{
	bool needsNormalise = false;

	Vector<String> parts;
	parts.reserve(pathParts.size() + other.pathParts.size());
	for (const auto& p: pathParts) {
		if (p != ".") {
			parts.push_back(p);
		}
	}
	for (const auto& p : other.pathParts) {
		parts.push_back(p);
		if (p == "..") {
			needsNormalise = true;
		}
	}
	return Path(std::move(parts), needsNormalise);
}

bool Path::operator==(const char* other) const
{
	return operator==(Path(other));
}

bool Path::operator==(const String& other) const 
{
	return operator==(Path(other));
}

bool Path::operator==(const Path& other) const 
{
	return *this == other.getParts();
}

bool Path::operator==(gsl::span<const String> other) const
{
	if (pathParts.size() != other.size()) {
		return false;
	}

	const size_t n = pathParts.size();
	for (size_t i = 0; i < n; ++i) {
		// Scan backwards as the end of the path is much more likely to be different
		auto& a = pathParts[n - i - 1];
		auto& b = other[n - i - 1];

#if defined(_WIN32) || defined(__APPLE__)
		if (a.size() != b.size() || !a.asciiCompareNoCase(b.c_str())) {
#else
		if (a != b) {
#endif
			return false;
		}
	}
	return true;
}

bool Path::operator!=(const Path& other) const 
{
	return !(*this == other);
}

bool Path::operator<(const Path& other) const
{
	return pathParts < other.pathParts;
}

bool Path::writeFile(const Path& path, gsl::span<const gsl::byte> data)
{
#ifdef _WIN32
	std::ofstream fp(path.getString().getUTF16().c_str(), std::ios::binary | std::ios::out);
#else
	std::ofstream fp(path.string(), std::ios::binary | std::ios::out);
#endif
	if (fp.is_open()) {
		fp.write(reinterpret_cast<const char*>(data.data()), data.size());
		fp.close();
		return true;
	}
	return false;
}

bool Path::writeFile(const Path& path, const Bytes& data)
{
	return writeFile(path, gsl::as_bytes(gsl::span<const Byte>(data)));
}

bool Path::writeFile(const Path& path, const String& data)
{
	return writeFile(path, gsl::as_bytes(gsl::span<const char>(data.c_str(), data.length())));
}

void Path::touchFile(const Path& path)
{
	utime(path.string().c_str(), nullptr);
}

bool Path::exists(const Path& path)
{
#if !defined(_LIBCPP_HAS_NO_FILESYSTEM_LIBRARY) && !defined(__NX_TOOLCHAIN_MAJOR__)
	std::error_code ec;
	return std::filesystem::exists(path.string(), ec);
#else
	return false;
#endif
}

Bytes Path::readFile(const Path& path)
{
	Bytes result;

#ifdef _WIN32
	std::ifstream fp(path.getString().getUTF16().c_str(), std::ios::binary | std::ios::in);
#else
	std::ifstream fp(path.string(), std::ios::binary | std::ios::in);
#endif
	if (!fp.is_open()) {
		return result;
	}

	fp.seekg(0, std::ios::end);
	const auto size = fp.tellg();
	fp.seekg(0, std::ios::beg);
	result.resize(size_t(size));

	fp.read(reinterpret_cast<char*>(result.data()), size);
	fp.close();

	return result;
}

String Path::readFileString(const Path& path)
{
	String result;

#ifdef _WIN32
	std::ifstream fp(path.getString().getUTF16().c_str(), std::ios::binary | std::ios::in);
#else
	std::ifstream fp(path.string(), std::ios::binary | std::ios::in);
#endif
	if (!fp.is_open()) {
		return result;
	}

	fp.seekg(0, std::ios::end);
	const auto size = fp.tellg();
	fp.seekg(0, std::ios::beg);
	result.setSize(size);

	fp.read(&result[0], size);
	fp.close();

	return result;	
}

Vector<String> Path::readFileLines(const Path& path)
{
	const auto bytes = readFile(path);
	if (bytes.empty()) {
		return {};
	}
	Vector<String> result;

	std::string_view remaining(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	while (!remaining.empty()) {
		auto end = remaining.find('\n');
		std::string_view current = remaining.substr(0, end);
		if (!current.empty() && current.back() == '\r') {
			current = current.substr(0, current.size() - 1);
		}
		remaining = remaining.substr(std::min(remaining.size(), end == std::string_view::npos ? end : end + 1));

		result.push_back(current);
	}

	return result;
}

void Path::removeFile(const Path& path)
{
	std::remove(path.string().c_str());
}

bool Path::isPrefixOf(const Path& other) const
{
	size_t n = std::min(pathParts.size(), other.pathParts.size());
	for (size_t i = 0; i < n; ++i) {
		if (pathParts[i] != other.pathParts[i] && pathParts[i] != ".") {
			return false;
		}
	}
	return true;
}

Path Path::makeRelativeTo(const Path& path) const
{
	const Path& me = *this;
	size_t sharedRoot = 0;
	size_t maxLen = std::min(me.pathParts.size(), path.pathParts.size());

	Vector<String> result;
	for (size_t i = 0; i < maxLen; ++i) {
		if (me.pathParts[i] == path.pathParts[i]) {
			sharedRoot = i + 1;
		} else {
			break;
		}
	}

	const bool relToDir = path.isDirectory();
	//const int foldersAbove = int(path.getNumberPaths()) - int(sharedRoot) - (isAbsolute() ? 0 : 1) - (relToDir ? 1 : 0);
	const int foldersAbove = int(path.getNumberPaths()) - int(sharedRoot) - (relToDir ? 1 : 0);
	for (int i = 0; i < foldersAbove; ++i) {
		result.emplace_back("..");
	}

	for (size_t i = sharedRoot; i < me.pathParts.size(); ++i) {
		result.emplace_back(me.pathParts[i]);
	}

	return Path(result, true);
}

Path Path::changeRelativeRoot(const Path& currentParent, const Path& newParent) const
{
	const auto absolute = isAbsolute() ? *this : (currentParent / (*this));
	return absolute.makeRelativeTo(newParent);
}

bool Path::isDirectory() const
{
	return !pathParts.empty() && pathParts.back() == ".";
}

bool Path::isFile() const
{
	return !pathParts.empty() && pathParts.back() != ".";
}

bool Path::isAbsolute() const
{
	if (pathParts.empty()) {
		return false;
	} else {
		return pathParts[0].endsWith(":") || pathParts[0].isEmpty();
	}
}

bool Path::isEmpty() const
{
	return pathParts.empty() || pathParts[0].isEmpty();
}

size_t Path::getHash() const
{
	Hash::Hasher hasher;
	for (const auto& p: pathParts) {
		hasher.feed(p);
	}
	return hasher.digest();
}

Path Path::getRoot() const
{
	return pathParts.front();
}

Path Path::getFront(size_t n) const
{
	if (n >= pathParts.size()) {
		return *this;
	}
	return Path(Vector<String>(pathParts.begin(), pathParts.begin() + n), true);
}

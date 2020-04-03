#include "halley/file/path.h"
#include <sstream>
#include <fstream>
#include "halley/os/os.h"

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

Path::Path(std::vector<String> parts)
	: pathParts(parts)
{
	normalise();
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
			if ((first || last) && canInsertDot()) {
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
	String filename = pathParts.back();
	if (filename == "." || filename == "..") {
		return filename;
	}
	size_t dotPos = filename.find_last_of('.');
	return filename.substr(dotPos);
}

String Path::getString() const
{
	std::stringstream s;
	bool first = true;
	for (auto& p : pathParts) {
		if (first) {
			first = false;
		} else {
			s << "/";
		}
		s << p;
	}
	return s.str();
}

String Path::getNativeString() const
{
	auto str = getString();
#ifdef _WIN32
	for (auto& c: str.cppStr()) {
		if (c == '/') {
			c = '\\';
		}
	}
#endif
	return str;
}

String Path::toString() const
{
	return getString();
}

size_t Path::getNumberPaths() const
{
	return pathParts.size();
}

Path Path::dropFront(int numberFolders) const
{
	return Path(std::vector<String>(pathParts.begin() + numberFolders, pathParts.end()));
}

Path Path::parentPath() const
{
	return Path(getString() + "/..");
}

Path Path::replaceExtension(String newExtension) const
{
	auto parts = pathParts;
	parts.back() = getStem().getString() + newExtension;
	return Path(parts);
}

Path Path::operator/(const char* other) const
{
	return operator/(Path(other));
}

Path Path::operator/(const Path& other) const 
{
	auto parts = pathParts;
	for (auto& p : other.pathParts) {
		parts.push_back(p);
	}
	return Path(parts);
}

Path Path::operator/(const String& other) const
{
	return operator/(Path(other));
}

Path Path::operator/(const std::string& other) const 
{
	return operator/(Path(other));
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
	return ! operator!=(other);
}

bool Path::operator!=(const Path& other) const 
{
	if (pathParts.size() != other.pathParts.size()) {
		return true;
	}

	for (size_t i = 0; i < pathParts.size(); ++i) {
		auto& a = pathParts[i];
		auto& b = other.pathParts[i];
#ifdef _WIN32
		if (a.asciiLower() != b.asciiLower()) {
#else
		if (a != b) {
#endif
			return true;
		}
	}
	return false;
}

std::string Path::string() const
{
	return getString().cppStr();
}

void Path::writeFile(const Path& path, const Bytes& data)
{
	OS::get().atomicWriteFile(path, data);
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

void Path::removeFile(const Path& path)
{
	std::remove(path.string().c_str());
}

Path Path::makeRelativeTo(const Path& path) const
{
	const Path& me = *this;
	size_t sharedRoot = 0;
	size_t maxLen = std::min(me.pathParts.size(), path.pathParts.size());

	std::vector<String> result;
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

	return Path(result);
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

std::ostream& Halley::operator<<(std::ostream& os, const Path& p)
{
	return (os << p.string());
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
	return Path(std::vector<String>(pathParts.begin(), pathParts.begin() + n));
}

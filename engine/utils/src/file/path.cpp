#include "halley/file/path.h"
#include <sstream>

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
	rawPath.replace("\\", "/");
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
			if (first || last) {
				write(current);
			}
		} else if (current == "..") {
			if (writePos > 0 && pathParts[writePos - 1] != "..") {
				--writePos;
				lastIsBack = true;
			} else {
				write(current);
			}
		} else {
			write(current);
		}
	}
	if (lastIsBack) {
		write(".");
	}

	pathParts.resize(writePos);
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
	String filename = getFilename().pathParts.front();
	if (filename == "." || filename == "..") {
		return filename;
	}
	size_t dotPos = filename.find('.');
	return filename.substr(0, dotPos);
}

String Path::getExtension() const
{
	String filename = getFilename().pathParts.front();
	if (filename == "." || filename == "..") {
		return filename;
	}
	size_t dotPos = filename.find('.');
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

std::ostream& Halley::operator<<(std::ostream& os, const Path& p)
{
	return (os << p.string());
}

Path Path::getRoot() const
{
	return pathParts.front();
}


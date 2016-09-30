#include "halley/file/path.h"
#include <boost/filesystem.hpp>

using namespace Halley;

Path::Path() {}

Path::Path(const char* name) : p(name) {}
Path::Path(const std::string& name) : p(name) {}
Path::Path(const String& name) : p(name) {}
Path::Path(const filesystem::path& path) : p(path.string()) {}

Path& Path::operator=(const std::string& other)
{
	p = other;
	return *this;
}

Path& Path::operator=(const String& other)
{
	p = other;
	return *this;
}

Path Path::getStem() const
{
	return getNative().stem();
}

String Path::getExtension() const
{
	return getNative().extension().string();
}

Path Path::parentPath() const
{
	return getNative().parent_path();
}

Path Path::replaceExtension(String newExtension) const
{
	auto p = getNative();
	p.replace_extension(newExtension.cppStr());
	return p;
}

Path Path::operator/(const char* other) const
{
	return operator/(Path(other));
}

Path Path::operator/(const Path& other) const 
{
	return getNative() / other.getNative();
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
	return p == other;
}

bool Path::operator==(const String& other) const 
{
	return p == other;
}

bool Path::operator==(const Path& other) const 
{
	return p == other.p;
}

bool Path::operator!=(const Path& other) const 
{
	return p != other.p;
}

std::string Path::string() const
{
	return getNative().string();
}

filesystem::path Path::getNative() const
{
	return filesystem::path(p.cppStr());
}

std::ostream& Halley::operator<<(std::ostream& os, const Path& p)
{
	return (os << p.string());
}

Path Path::getRoot() const
{
	return filesystem::path(p.cppStr()).begin()->string();
}

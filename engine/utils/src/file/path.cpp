#include "halley/file/path.h"
#include <boost/filesystem.hpp>

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

Path::Path(const filesystem::path& path)
{
	setPath(path.string());
}

Path::Path(const String& name)
{
	setPath(name);
}

void Path::setPath(const String& value)
{
	String str = filesystem::path(value.cppStr()).lexically_normal().string();
#ifdef _WIN32
	str.replace("/", "\\");
#endif
	p = str;	
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
	auto n = getNative();
	n.replace_extension(newExtension.cppStr());
	return n;
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
	return operator==(Path(other));
}

bool Path::operator==(const String& other) const 
{
	return operator==(Path(other));
}

bool Path::operator==(const Path& other) const 
{
#ifdef _WIN32
	return p.asciiLower() == other.p.asciiLower();
#else
	return p == other.p;
#endif
}

bool Path::operator!=(const Path& other) const 
{
#ifdef _WIN32
	return p.asciiLower() != other.p.asciiLower();
#else
	return p == other.p;
#endif
}

std::string Path::string() const
{
	return p;
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
	return getNative().begin()->string();
}

#pragma once

#include "halley/text/halleystring.h"
#include <iostream>

namespace boost
{
	namespace filesystem
	{
		class path;
	}
}

namespace Halley
{
	namespace filesystem = boost::filesystem;
	
	class Path
	{
	public:
		Path();
		Path(const char* name);
		Path(const std::string& name);
		Path(const String& name);
		Path(const filesystem::path& path);

		Path& operator=(const std::string& other);
		Path& operator=(const String& other);

		Path getRoot() const;
		Path getStem() const;
		String getExtension() const;

		Path parentPath() const;
		Path replaceExtension(String newExtension) const;

		Path operator/(const char* other) const;
		Path operator/(const Path& other) const;
		Path operator/(const String& other) const;
		Path operator/(const std::string& other) const;

		bool operator==(const char* other) const;
		bool operator==(const String& other) const;
		bool operator==(const Path& other) const;

		bool operator!=(const Path& other) const;

		std::string string() const;

		filesystem::path getNative() const;

	private:
		String p;
		void setPath(const String& value);
	};

	std::ostream& operator<<(std::ostream& os, const Path& p);
}

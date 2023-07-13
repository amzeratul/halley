#pragma once

#include "halley/text/halleystring.h"
#include "halley/utils/utils.h"
#include <gsl/span>

namespace Halley
{
	class Path
	{
	public:
		Path();
		Path(const char* name);
		Path(const std::string& name);
		Path(const String& name);

		Path(const Path& other) = default;
		Path(Path&& other) noexcept = default;
		Path& operator=(const Path& other) = default;
		Path& operator=(Path&& other) noexcept = default;

		Path& operator=(const std::string& other);
		Path& operator=(const String& other);

		Path getRoot() const;
		Path getFront(size_t n) const;
		Path getFilename() const;
		Path getDirName() const;
		Path getStem() const;
		String getExtension() const;
		String getString(bool includeDot = true) const;
		String getNativeString(bool includeDot = true) const;
		String toString() const;

		gsl::span<const String> getParts() const;
		size_t getNumberPaths() const;

		Path dropFront(int numberFolders) const;

		Path parentPath() const;
		Path replaceExtension(String newExtension) const;

		Path operator/(const char* other) const;
		Path operator/(const Path& other) const;
		Path operator/(const String& other) const;
		Path operator/(const std::string& other) const;

		bool operator==(const char* other) const;
		bool operator==(const String& other) const;
		bool operator==(const Path& other) const;
		bool operator==(gsl::span<const String> other) const;

		bool operator!=(const Path& other) const;
		bool operator<(const Path& other) const;

		std::string string() const;

		static bool writeFile(const Path& path, gsl::span<const gsl::byte> data);
		static bool writeFile(const Path& path, const Bytes& data);
		static bool writeFile(const Path& path, const String& data);

		static bool exists(const Path& path);
		static Bytes readFile(const Path& path);
		static String readFileString(const Path& path);
		static Vector<String> readFileLines(const Path& path);

		static void removeFile(const Path& path);

		bool isPrefixOf(const Path& other) const;
		Path makeRelativeTo(const Path& path) const;
		Path changeRelativeRoot(const Path& currentParent, const Path& newParent) const;

		bool isDirectory() const;
		bool isFile() const;
		bool isAbsolute() const;
		bool isEmpty() const;

		size_t getHash() const;

	private:
		Vector<String> pathParts;
		void normalise();
		void setPath(const String& value);
		std::string makeString(bool includeDot, char dirSeparator) const;

		explicit Path(Vector<String> parts);
	};

	using TimestampedPath = std::pair<Path, int64_t>;
}

namespace std {
	template<>
	struct hash<Halley::Path>
	{
		size_t operator()(const Halley::Path& v) const noexcept
		{
			return v.getHash();
		}
	};
}

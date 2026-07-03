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
		Path(std::string name);
		Path(String name);

		Path(const Path& other) = default;
		Path(Path&& other) noexcept = default;
		Path& operator=(const Path& other) = default;
		Path& operator=(Path&& other) noexcept = default;

		Path& operator=(const std::string& other);
		Path& operator=(String other);

		std::string_view getRoot() const;
		Path getFront(size_t n) const;
		std::string_view getFilename() const;
		String getFilenameStr() const;
		std::string_view getDirName() const;
		String getDirNameStr() const;
		std::string_view getStem() const;
		String getStemStr() const;
		std::string_view getExtension() const;
		String getExtensionStr() const;
		std::string_view getStringView(bool includeDot = true) const;
		String getString(bool includeDot = true) const;
		String getNativeString(bool includeDot = true) const;
		String toString() const;

		size_t getNumberOfParts() const;
		std::string_view getPart(size_t idx) const;

		Path dropFront(int numberFolders) const;

		Path parentPath() const;
		Path replaceExtension(std::string_view newExtension) const;

		Path operator/(std::string_view other) const;
		Path operator/(const char* other) const;
		Path operator/(const String& other) const;
		Path operator/(const std::string& other) const;
		Path operator/(const Path& other) const;

		bool operator==(const char* other) const;
		bool operator==(const String& other) const;
		bool operator==(const Path& other) const;
		bool operator!=(const Path& other) const;
		bool operator<(const Path& other) const;

		std::string string() const;

		static bool writeFile(const Path& path, gsl::span<const std::byte> data);
		static bool writeFile(const Path& path, const Bytes& data);
		static bool writeFile(const Path& path, const String& data);
		static void touchFile(const Path& path);

		static bool exists(const Path& path);
		static void rename(const Path& from, const Path& to);
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

		Vector<Path> enumerateDirectory(bool makeRelative) const;

		void makeLowerCase();
		static bool isCaseSensitive();

	private:
		String str;

		void normalise();
		static std::string_view normalise(gsl::span<char> buffer, std::string_view str);
		void setPath(String value);

		std::string_view getFrontParts(size_t n) const;
		std::string_view getLastPart() const;
		size_t getLastPartPos() const;
		std::pair<std::string_view, std::string_view> getLastTwoParts() const;
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

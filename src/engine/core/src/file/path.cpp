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
#ifdef __PROSPERO__
	#include "kernel.h"
#else
	#include <utime.h>
#endif // __PROSPERO__
#endif // _MSC_VER

#include "halley/os/os.h"
#include "halley/utils/hash.h"

using namespace Halley;

Path::Path()
{}

Path::Path(const char* name, bool normalise)
{
	setPath(std::string_view(name), normalise);
}

Path::Path(std::string_view name, bool normalise)
{
	setPath(name, normalise);
}

Path::Path(std::string name, bool normalise)
{
	setPath(String(std::move(name)), normalise);
}

Path::Path(String name, bool shouldNormalise)
{
	setPath(std::move(name), shouldNormalise);
}

void Path::setPath(std::string_view value, bool shouldNormalise)
{
	if (shouldNormalise) {
		std::array<char, 2048> buffer;
		str = String(normalise(buffer, std::string_view(value)));
	} else {
		str = String(value);
	}
	computeProperties();
}

void Path::setPath(String value, bool shouldNormalise)
{
	str = std::move(value);
	if (shouldNormalise) {
		std::array<char, 2048> buffer;
		auto n = normalise(buffer, str);
		if (n != str) {
			str = String(n);
		}
	}
	computeProperties();
}

std::string_view Path::normalise(gsl::span<char> buffer, std::string_view str)
{
	// Normalises the path:
	// Remove ".." plus whatever directory comes before: foo/bar/../baz -> foo/baz
	// ...but keep any at the front: ../../foo
	// Remove ".": foo/bar/./baz -> foo/bar/baz
	// ...but keep it at the end: /foo/.
	// Ensure directories end in a ".": foo/bar/ -> foo/bar/.
	// Collapse double "/": foo//bar -> foo/bar

	HalleyAssertDev(buffer.size() >= str.size() + 1);
	std::array<char, 2048> winBuf;

	// Some preprocessing on Windows
	if (getPlatform() == GamePlatform::Windows) {
		HalleyAssertDev(winBuf.size() >= str.size());
		// Convert backslashes
		for (size_t i = 0; i < str.length(); ++i) {
			const auto c = str[i];
			winBuf[i] = c == '\\' ? '/' : c;
		}

		// Also make sure the drive name is uppercase
		if (str.length() >= 2 && winBuf[1] == ':' && winBuf[0] >= 'a' && winBuf[0] <= 'z') {
			winBuf[0] -= 32; // Make uppercase
		}

		str = std::string_view(winBuf.data(), str.length());
	}

	const bool isDir = str.ends_with("/") || str.ends_with("/.") || str.ends_with("/..");

	// Split string
	std::array<std::string_view, 64> partsBuffer;
	const auto parts = String::splitToBuffer(str, '/', partsBuffer);

	std::array<std::string_view, 64> resultParts;
	size_t nResultParts = 0;
	auto addPart = [&] (std::string_view p)
	{
		resultParts.at(nResultParts++) = p;
	};
	auto removeLastPart = [&] () -> bool
	{
		if (nResultParts >= 1 && resultParts[nResultParts - 1] != "..") {
			--nResultParts;
			return true;
		} else {
			return false;
		}
	};

	// Generate result parts
	for (size_t i = 0; i < parts.size(); ++i) {
		const auto& part = parts[i];
		const bool isLast = i + 1 == parts.size();

		if (part.empty() && i == 0) {
			addPart(part);
		} else if (part.empty() || part == ".") {
			if (isLast) {
				addPart(".");
			}
		} else {
			if (part == "..") {
				if (!removeLastPart()) {
					addPart(part);
				}
			} else {
				addPart(part);
			}
		}
	}

	if (isDir && (nResultParts == 0 || (resultParts[nResultParts - 1] != "." && resultParts[nResultParts - 1] != ".."))) {
		addPart(".");
	}

	// Make buffer
	return String::concatStringViewsInBuffer(buffer, gsl::span(resultParts).subspan(0, nResultParts), "/");
}

Path& Path::operator=(std::string other)
{
	setPath(String(std::move(other)));
	return *this;
}

Path& Path::operator=(std::string_view other)
{
	setPath(other);
	return *this;
}

Path& Path::operator=(String other)
{
	setPath(std::move(other));
	return *this;
}

std::string_view Path::getFilenameStrView() const
{
	auto part = getLastPart();
	if (part == ".") {
		return {};
	}
	return part;
}

String Path::getFilename() const
{
	return getFilenameStrView();
}

std::string_view Path::getDirNameStrView() const
{
	const auto [prev, cur] = getLastTwoParts();
	if (cur == ".") {
		return prev;
	}
	return "";
}

String Path::getDirName() const
{
	return getDirNameStrView();
}

std::string_view Path::getStemStrView() const
{
	const auto filename = getFilenameStrView();
	if (filename == "." || filename == "..") {
		return filename;
	}
	size_t dotPos = filename.find_last_of('.');
	return filename.substr(0, dotPos);
}

String Path::getStem() const
{
	return getStemStrView();
}

std::string_view Path::getExtensionStrView() const
{
	const auto filename = getFilenameStrView();
	if (filename == "." || filename == "..") {
		return filename;
	}
	const size_t dotPos = filename.find_last_of('.');
	if (dotPos == std::string_view::npos) {
		return {};
	}
	return filename.substr(dotPos);
}

String Path::getExtension() const
{
	return getExtensionStrView();
}

std::string_view Path::getPart(size_t idx) const
{
	auto s = std::string_view(str);
	size_t startPos = 0;
	size_t nFound = 0;
	for (size_t i = 0; nFound < idx && i < s.length(); ++i) {
		if (s[i] == '/') {
			++nFound;
		}
	}

	for (size_t i = 0; i < idx; ++i) {
		startPos = s.find('/', startPos);
		if (startPos == std::string_view::npos) {
			return {};
		}
		startPos += 1; // Skip slash
	}

	size_t endPos = s.find('/', startPos);
	if (endPos == std::string_view::npos) {
		return s.substr(startPos);
	} else {
		return s.substr(startPos, endPos - startPos);
	}
}

std::string_view Path::getFrontParts(size_t n) const
{
	if (n == 0) {
		return {};
	}

	const size_t len = str.length();
	size_t nSlashes = 0;
	for (size_t i = 0; i < len; ++i) {
		if (str[i] == '/') {
			++nSlashes;
			if (nSlashes == n) {
				return std::string_view(str).substr(0, i + 1);
			}
		}
	}
	return std::string_view(str);
}

std::string_view Path::getLastPart() const
{
	return std::string_view(str).substr(getLastPartPos());
}

size_t Path::getLastPartPos() const
{
	const auto p = str.find_last_of('/');
	if (p == String::npos) {
		return 0;
	} else {
		return p + 1;
	}
}

std::pair<std::string_view, std::string_view> Path::getLastTwoParts() const
{
	auto s = std::string_view(str);
	size_t startPos = 0;
	std::string_view prev;
	while (true) {
		startPos = s.find('/', startPos);
		auto cur = s.substr(startPos);
		if (startPos == std::string_view::npos) {
			return { prev, cur };
		}
		startPos += 1; // Skip slash
		prev = cur;
	}
}

void Path::computeProperties()
{
	if (str.isEmpty()) {
		numberOfParts = 0;
		isDir = false;
		return;
	}

	size_t n = 1;
	for (auto c: std::string_view(str)) {
		if (c == '/') {
			++n;
		}
	}
	numberOfParts = n;

	isDir = str.endsWith("/") || str.endsWith("/.") || str.endsWith("/..") || str == "." || str == "..";
}

size_t Path::getNumberOfParts() const
{
	return numberOfParts;
}

std::string Path::string() const
{
	return std::string(getStringView(true));
}

std::string_view Path::getStringView(bool includeDot) const
{
	auto result = std::string_view(str);
	if (!includeDot && result.ends_with("/.")) {
		result = result.substr(0, result.length() - 2);
	}
	return result;
}

String Path::getString(bool includeDot) const
{
	return String(getStringView(includeDot));
}

String Path::getNativeString(bool includeDot) const
{
	constexpr char separator = getPlatform() == GamePlatform::Windows ? '\\' : '/';

	String str = getString(includeDot);
	if (separator != '/') {
		for (auto& c: str.cppStr()) {
			if (c == '/') {
				c = separator;
			}
		}
	}
	return str;
}

String Path::toString() const
{
	return getString(true);
}

Path Path::dropFront(int numberFolders) const
{
	auto toDrop = getFrontParts(numberFolders);
	return Path(str.substr(toDrop.length()));
}

Path Path::parentPath() const
{
	size_t toDrop = isDirectory() ? 2 : 1;
	const auto n = getNumberOfParts();
	return getFront(std::max<size_t>(n, toDrop) - toDrop);
}

Path Path::replaceExtension(std::string_view newExtension) const
{
	auto filenamePos = getLastPartPos();
	const size_t dotPos = std::string_view(str).substr(filenamePos).find_last_of('.') + filenamePos;
	if (dotPos == std::string_view::npos) {
		return Path(str + newExtension);
	} else {
		return Path(str.substr(0, dotPos) + newExtension);
	}
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

Path Path::operator/(std::string_view other) const
{
	std::array<char, 2048> buffer1;
	auto result = String::concatInBuffer(buffer1, getStringView(false), "/", other);
	return Path(result, other.starts_with(".") || other.starts_with(".."));
}

Path Path::operator/(const Path& other) const 
{
	return operator/(other.getStringView(true));
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
	return str == other.str;
}

bool Path::operator!=(const Path& other) const 
{
	return !(*this == other);
}

bool Path::operator<(const Path& other) const
{
	return str < other.str;
}

bool Path::writeFile(const Path& path, gsl::span<const std::byte> data)
{
#ifdef _WIN32
	std::ofstream fp(path.str.getUTF16().c_str(), std::ios::binary | std::ios::out);
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
#ifdef __PROSPERO__
	sceKernelUtimes(path.string().c_str(), nullptr);
#else
	utime(path.string().c_str(), nullptr);
#endif
}

bool Path::exists(const Path& path)
{
#if !defined(_LIBCPP_HAS_NO_FILESYSTEM_LIBRARY) && !defined(NN_NINTENDO_SDK)
	std::error_code ec;
	return std::filesystem::exists(path.string(), ec);
#else
	return false;
#endif
}

void Path::rename(const Path& from, const Path& to)
{
#if !defined(_LIBCPP_HAS_NO_FILESYSTEM_LIBRARY) && !defined(NN_NINTENDO_SDK)
	std::error_code ec;

	if (!exists(to.parentPath())) {
		std::filesystem::create_directories(to.parentPath().string(), ec);
		if (ec) {
			Logger::logError(ec.message());
			return;
		}
	}

	std::filesystem::rename(from.string(), to.string(), ec);
	if (ec) {
		Logger::logError(ec.message());
	}
#else
	return;
#endif
}

Bytes Path::readFile(const Path& path)
{
	Bytes result;

#ifdef _WIN32
	std::ifstream fp(path.str.getUTF16().c_str(), std::ios::binary | std::ios::in);
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
	std::ifstream fp(path.str.getUTF16().c_str(), std::ios::binary | std::ios::in);
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
	auto name = getStringView(false);
	return other.str.startsWith(name);
}

Path Path::makeRelativeTo(const Path& path) const
{
	std::array<std::string_view, 64> myPartsBuffer;
	const auto myParts = String::splitToBuffer(str, '/', myPartsBuffer);
	std::array<std::string_view, 64> theirPartsBuffer;
	const auto theirParts = String::splitToBuffer(path.str, '/', theirPartsBuffer);

	size_t sharedRoot = 0;
	size_t maxLen = std::min(myParts.size(), theirParts.size());

	for (size_t i = 0; i < maxLen; ++i) {
		if (myParts[i] == theirParts[i]) {
			sharedRoot = i + 1;
		} else {
			break;
		}
	}

	std::array<std::string_view, 64> resultBuffer;
	size_t pos = 0;

	const bool relToDir = path.isDirectory();
	const int foldersAbove = int(theirParts.size()) - int(sharedRoot) - (relToDir ? 1 : 0);
	for (int i = 0; i < foldersAbove; ++i) {
		resultBuffer.at(pos++) = "..";
	}

	for (size_t i = sharedRoot; i < myParts.size(); ++i) {
		resultBuffer.at(pos++) = myParts[i];
	}

	std::array<char, 2048> buffer;
	return Path(String::concatStringViewsInBuffer(buffer, gsl::span(resultBuffer).subspan(0, pos), "/"));
}

Path Path::changeRelativeRoot(const Path& currentParent, const Path& newParent) const
{
	const auto absolute = isAbsolute() ? *this : (currentParent / (*this));
	return absolute.makeRelativeTo(newParent);
}

bool Path::isDirectory() const
{
	return isDir;
}

bool Path::isFile() const
{
	const auto filename = getFilenameStrView();
	return !filename.empty() && filename != ".";
}

bool Path::isAbsolute() const
{
	if (isEmpty()) {
		return false;
	} else {
		const auto& root = getPart(0);
		return root.ends_with(":") || root.empty();
	}
}

bool Path::isEmpty() const
{
	return str.isEmpty();
}

size_t Path::getHash() const
{
	Hash::Hasher hasher;
	hasher.feed(str);
	return hasher.digest();
}

std::string_view Path::getRoot() const
{
	return getPart(0);
}

std::string_view Path::getFrontStrView(size_t n) const
{
	// This doesn't return the EXACT same as getFront(), as it won't return a trailing . for dir paths
	return getFrontParts(n);
}

Path Path::getFront(size_t n) const
{
	auto s = getFrontParts(n);
	if (s.empty()) {
		return {};
	} else if (s.length() != str.length()) {
		std::array<char, 2048> buffer;
		return Path(String::concatInBuffer(buffer, s, "."), false);
	} else {
		return *this;
	}
}



#if __has_include(<filesystem>)
#include <filesystem>
#ifdef __cpp_lib_filesystem
#define HAS_FILESYSTEM
#endif
#endif

#ifdef HAS_FILESYSTEM

namespace {
	std::filesystem::path getNative(const Path& p)
	{
#ifdef _WIN32
		return std::filesystem::path(p.getNativeString().getUTF16().c_str());
#else
		return std::filesystem::path(p.string());
#endif
	}
}

Vector<Path> Path::enumerateDirectory(bool makeRelative) const
{
	Vector<Path> result;
	if (exists(*this)) {
		std::filesystem::recursive_directory_iterator end;
		const auto dir = getNative(*this);
		for (auto i = std::filesystem::recursive_directory_iterator(dir); i != end; ++i) {
			std::filesystem::path fullPath = i->path();
			std::error_code ec;
			if (std::filesystem::is_regular_file(fullPath.native(), ec)) {
				if (makeRelative) {
					result.push_back(Path(String(StringUTF32(fullPath.lexically_relative(dir).u32string()))));
				} else {
					result.push_back(Path(String(StringUTF32(fullPath.u32string()))));
				}
			}
		}
	}
	return result;
}

void Path::makeLowerCase()
{
	str.asciiMakeLower();
}

bool Path::isCaseSensitive()
{
	return getPlatform() != GamePlatform::Windows && getPlatform() != GamePlatform::MacOS;
}

#else

Vector<Path> Path::enumerateDirectory(bool makeRelative) const
{
	Logger::logError("Path::enumerateDirectory() is not implemented due to missing filesystem library");
	return {};
}

#endif

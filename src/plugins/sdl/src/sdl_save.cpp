#include "sdl_save.h"
#include "halley/os/os.h"
#include "halley/support/logger.h"
#include "halley/maths/random.h"
#include "halley/utils/encrypt.h"
#include "halley/utils/hash.h"
using namespace Halley;

SDLSaveHeaderV0::SDLSaveHeaderV0()
{
	memcpy(formatId.data(), "HLLYSAVE", 8);
}

size_t SDLSaveHeader::read(gsl::span<const gsl::byte> data)
{
	size_t pos = 0;
	auto readData = [&] (void* dst, size_t dstSize)
	{
		if (size_t(data.size()) >= pos + dstSize) {
			memcpy(dst, data.data() + pos, dstSize);
			pos += dstSize;
		}
	};

	v0.version = -1;

	if (size_t(data.size()) < sizeof(SDLSaveHeaderV0)) {
		return 0;
	}
	
	// V0
	readData(&v0, sizeof(SDLSaveHeaderV0));
	if (!isValidHeader()) {
		return 0;
	}

	// V1
	if (v0.version >= 1) {
		readData(&v1, sizeof(SDLSaveHeaderV1));
	}

	return pos;
}

bool SDLSaveHeader::isValidHeader() const
{
	if (v0.version < 0) {
		// Invalid version
		return false;
	}
	if (v0.version > 1) {
		// Future version
		return false;
	}
	if (memcmp(v0.formatId.data(), "HLLYSAVE", 8) != 0) {
		// Not even a Halley save
		return false;
	}
	if (v0.reserved != 0) {
		// Future version?
		return false;
	}
	return true;
}

bool SDLSaveHeader::isValid(const String& path, const String& key) const
{
	if (!isValidHeader()) {
		return false;
	}
	if (v0.fileNameHash != computeHash(path, key)) {
		// Corrupt
		return false;
	}

	return true;
}

void SDLSaveHeader::generateIV()
{
	Random::getGlobal().getBytes(gsl::as_writable_bytes(gsl::span<char>(v0.iv.data(), v0.iv.size())));
}

Bytes SDLSaveHeader::getIV() const
{
	Bytes result;
	result.resize(v0.iv.size());
	memcpy(result.data(), v0.iv.data(), v0.iv.size());
	return result;
}

uint64_t SDLSaveHeader::computeHash(const String& path, const String& key)
{
	String filename = path + ":" + key;
	return Hash::hash(gsl::as_bytes(gsl::span<const char>(filename.c_str(), filename.length())));
}

SDLSaveData::SDLSaveData(SaveDataType type, Path dir, std::optional<String> key)
	: type(type)
	, dir(std::move(dir))
	, key(std::move(key))
{
	OS::get().createDirectories(this->dir);
}

bool SDLSaveData::isReady() const
{
	return true;
}

Bytes SDLSaveData::getData(const String& filename)
{
	Expects (!filename.isEmpty());

	auto path = dir / filename;
	std::optional<Bytes> data = doGetData(path, filename);
	if (data) {
		return *data;
	} else {
		// Fallback to backup
		data = doGetData(path.replaceExtension(path.getExtension() + ".bak"), filename);
		if (data) {
			return *data;
		} else {
			return {};
		}
	}
}

void SDLSaveData::removeData(const String& path)
{
	Expects (!path.isEmpty());
	Path::removeFile(dir / path);
	auto backupFile = dir / path;
	backupFile = backupFile.replaceExtension(backupFile.getExtension() + ".bak");
	Path::removeFile(backupFile);
}

Vector<String> SDLSaveData::enumerate(const String& root)
{
	auto paths = OS::get().enumerateDirectory(dir);
	Vector<String> result;
	for (auto& p: paths) {
		auto path = p.toString();
		if (path.startsWith(root) && !path.endsWith(".bak")) {
			result.push_back(path);
		}
	}
	return result;
}

void SDLSaveData::setData(const String& path, const Bytes& rawData, bool commit)
{
	Expects (!path.isEmpty());

	Bytes finalData;
	const bool encrypted = static_cast<bool>(key);

	if (encrypted) {
		// Encrypt
		auto k = getKey();
		SDLSaveHeader header;
		header.generateIV();
		header.v0.fileNameHash = SDLSaveHeader::computeHash(path, k);
		header.v1.dataHash = Hash::hash(rawData);
		auto encryptedData = Encrypt::encrypt(header.getIV(), k, rawData);
		
		// Pack
		finalData.resize(sizeof(header) + encryptedData.size());
		memcpy(finalData.data(), &header, sizeof(header));
		memcpy(finalData.data() + sizeof(header), encryptedData.data(), encryptedData.size());
	} else {
		finalData = rawData;
	}

	// Paths
	auto dstPath = dir / path;
	auto dstPathStr = dstPath.getString();
	std::optional<Path> backupPath;
	if (corruptedFiles.find(dstPathStr) != corruptedFiles.end()) {
		// File we're writing to was corrupted; don't back up, but do remove it from the list
		corruptedFiles.erase(dstPathStr);
	} else {
		// We've read from this file safely before, so back it up!
		// But don't do it for downloads, those don't count as highly sensitive data
		if (type != SaveDataType::Downloads) {
			backupPath = dstPath.replaceExtension(dstPath.getExtension() + ".bak");
		}
	}

	// Write
	OS::get().createDirectories(dir);
	OS::get().atomicWriteFile(dstPath, gsl::as_bytes(gsl::span<const Byte>(finalData)), backupPath);
	Logger::logDev("Saving \"" + path + "\", " + String::prettySize(finalData.size()));
}

void SDLSaveData::commit()
{
}

String SDLSaveData::getKey() const
{
	if (key) {
		return key.value() + ":" + toString(type);
	} else {
		return "";
	}
}

std::optional<Bytes> SDLSaveData::doGetData(const Path& path, const String& filename)
{
	auto rawData = Path::readFile(path);
	if (rawData.empty()) {
		return {};
	}

	if (!key) {
		return rawData;
	}

	// Read header
	SDLSaveHeader header;
	const size_t headerSize = header.read(gsl::as_bytes(gsl::span<Byte>(rawData.data(), rawData.size())));
	if (headerSize == 0) {
		return rawData;
	}

	// Basic header validation
	const auto k = getKey();
	if (!header.isValid(filename, k)) {
		Logger::logError("Invalid save file: " + filename);
		return {};
	}

	// Decrypt data
	rawData.erase(rawData.begin(), rawData.begin() + headerSize);
	auto finalData = Encrypt::decrypt(header.getIV(), k, rawData);

	// Final validation
	if (header.v0.version >= 1 && header.v1.dataHash != Hash::hash(finalData)) {
		Logger::logError("Corrupted save file: " + filename);
		if (!path.getExtension().endsWith(".bak")) {
			corruptedFiles.insert(path.getString());
		}
;		return {};
	}
	return finalData;
}

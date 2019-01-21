#include "sdl_save.h"
#include "halley/os/os.h"
#include "halley/support/logger.h"
#include "halley/maths/random.h"
#include "halley/utils/encrypt.h"
#include "halley/utils/hash.h"
using namespace Halley;

bool SDLSaveHeader::isValid() const
{
	if (memcmp(formatId.data(), "HLLYSAVE", 8) != 0) {
		return false;
	}
	if (version > 0) {
		return false;
	}
	if (reserved != 0) {
		return false;
	}

	return true;
}

void SDLSaveHeader::init()
{
	memcpy(formatId.data(), "HLLYSAVE", 8);
	version = 0;
	reserved = 0;
	fileNameHash = 0;
}

void SDLSaveHeader::generateIV()
{
	Random::getGlobal().getBytes(gsl::as_writeable_bytes(gsl::span<char>(iv.data(), iv.size())));
}

Bytes SDLSaveHeader::getIV() const
{
	Bytes result;
	result.resize(iv.size());
	memcpy(result.data(), iv.data(), iv.size());
	return result;
}

uint64_t SDLSaveHeader::computeHash(const String& path, const String& key)
{
	String filename = Path(path).getFilename().getString() + ":" + key;
	return Hash::hash(gsl::as_bytes(gsl::span<const char>(filename.c_str(), filename.length())));
}

SDLSaveData::SDLSaveData(SaveDataType type, Path dir, Maybe<String> key)
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

Bytes SDLSaveData::getData(const String& path)
{
	Expects (!path.isEmpty());
	
	auto rawData = Path::readFile(dir / path);
	if (!key) {
		return rawData;
	}

	if (rawData.size() < sizeof(SDLSaveHeader)) {
		return rawData;
	}

	SDLSaveHeader header;
	memcpy(&header, rawData.data(), sizeof(header));
	if (!header.isValid()) {
		return rawData;
	}
	rawData.erase(rawData.begin(), rawData.begin() + sizeof(header));

	auto k = getKey();
	if (header.fileNameHash != SDLSaveHeader::computeHash(path, k)) {
		return {};
	}

	// Decrypt data
	return Encrypt::decrypt(header.getIV(), k, rawData);
}

void SDLSaveData::removeData(const String& path)
{
	Expects (!path.isEmpty());
	return Path::removeFile(dir / path);
}

std::vector<String> SDLSaveData::enumerate(const String& root)
{
	auto paths = OS::get().enumerateDirectory(dir);
	std::vector<String> result;
	for (auto& p: paths) {
		auto path = p.toString();
		if (path.startsWith(root)) {
			result.push_back(path);
		}
	}
	return result;
}

void SDLSaveData::setData(const String& path, const Bytes& rawData, bool commit)
{
	Expects (!path.isEmpty());

	Bytes finalData;
	constexpr bool encrypted = true;

	if (encrypted) {
		// Encrypt
		auto k = getKey();
		SDLSaveHeader header;
		header.init();
		header.generateIV();
		header.fileNameHash = SDLSaveHeader::computeHash(path, k);
		auto encryptedData = Encrypt::encrypt(header.getIV(), k, rawData);
		
		// Pack
		finalData.resize(sizeof(header) + encryptedData.size());
		memcpy(finalData.data(), &header, sizeof(header));
		memcpy(finalData.data() + sizeof(header), encryptedData.data(), encryptedData.size());
	} else {
		finalData = rawData;
	}

	// Write
	OS::get().createDirectories(dir);
	OS::get().atomicWriteFile(dir / path, finalData);
	Logger::logDev("Saving \"" + path + "\", " + String::prettySize(finalData.size()));
}

void SDLSaveData::commit()
{
}

String SDLSaveData::getKey() const
{
	if (key) {
		return key.get() + ":" + toString(type);
	} else {
		return "";
	}
}

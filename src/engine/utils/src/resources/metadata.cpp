#include "halley/resources/metadata.h"
#include "halley/resources/resource_data.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/text/string_converter.h"
#include "halley/file_formats/config_file.h"

using namespace Halley;

Metadata::Metadata() {}

Metadata::~Metadata() {}

bool Metadata::hasKey(const String& key) const
{
	return entries.find(key) != entries.end();
}

bool Metadata::getBool(const String& key) const
{
	return getString(key) == "true";
}

int Metadata::getInt(const String& key) const
{
	return getString(key).toInteger();
}

float Metadata::getFloat(const String& key) const
{
	return getString(key).toFloat();
}

String Metadata::getString(const String& key) const
{
	auto result = entries.find(key);
	if (result != entries.end()) {
		return result->second;
	} else {
		throw Exception("Key " + key + " not found in metafile.", HalleyExceptions::Resources);
	}
}

bool Metadata::getBool(const String& key, bool v) const
{
	if (hasKey(key)) {
		return getBool(key);
	} else {
		return v;
	}
}

int Metadata::getInt(const String& key, int v) const
{
	if (hasKey(key)) {
		return getInt(key);
	} else {
		return v;
	}
}

float Metadata::getFloat(const String& key, float v) const
{
	if (hasKey(key)) {
		return getFloat(key);
	} else {
		return v;
	}
}

String Metadata::getString(const String& key, String v) const
{
	if (hasKey(key)) {
		return getString(key);
	} else {
		return v;
	}
}

const std::map<String, String>& Metadata::getEntries() const
{
	return entries;
}

bool Metadata::set(String key, bool value)
{
	return set(std::move(key), value ? "true" : "false");
}

bool Metadata::set(String key, int value)
{
	return set(std::move(key), Halley::toString(value));
}

bool Metadata::set(String key, float value)
{
	return set(std::move(key), Halley::toString(value));
}

bool Metadata::set(String key, const char* value)
{
	return set(std::move(key), String(value));
}

bool Metadata::set(String key, const std::string& value)
{
	return set(std::move(key), String(value));
}

bool Metadata::set(String key, String value)
{
	const auto iter = entries.find(key);
	if (iter != entries.end()) {
		if (iter->second == value) {
			return false;
		}
		iter->second = std::move(value);
	} else {
		entries[std::move(key)] = std::move(value);
	}
	return true;
}

bool Metadata::erase(const String& key)
{
	const auto iter = entries.find(key);
	if (iter != entries.end()) {
		entries.erase(iter);
		return true;
	}
	return false;
}

void Metadata::convertToLatestVersion()
{
	if (hasKey("defaultMaterial") && !hasKey("material")) {
		set("material", getString("defaultMaterial"));
	}
}

std::unique_ptr<Metadata> Metadata::fromBinary(ResourceDataStatic& data)
{
	auto meta = std::make_unique<Metadata>();
	Deserializer s(data.getSpan());
	meta->deserialize(s);
	return meta;
}

void Metadata::serialize(Serializer& s) const
{
	s << entries;
}

void Metadata::deserialize(Deserializer& s)
{
	s >> entries;
}

bool Metadata::operator==(const Metadata& rhs) const
{
	return entries == rhs.entries;
}

bool Metadata::operator!=(const Metadata& rhs) const
{
	return entries != rhs.entries;
}

String Metadata::toString() const
{
	std::stringstream ss;
	ss << "{ ";
	for (auto& e: entries) {
		ss << "\"" << e.first << "\": \"" << e.second << "\" ";
	}
	ss << "}";
	return ss.str();
}

ConfigNode Metadata::toConfig() const
{
	ConfigNode::MapType result;
	for (auto& e: entries) {
		result[e.first] = e.second;
	}
	return ConfigNode(std::move(result));
}

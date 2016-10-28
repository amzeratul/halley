#include "halley/resources/metadata.h"
#include "halley/resources/resource_data.h"
#include "halley/file/byte_serializer.h"
#include "halley/text/string_converter.h"

using namespace Halley;

Metadata::Metadata() {}

Metadata::~Metadata() {}

bool Metadata::hasKey(String key) const
{
	return entries.find(key) != entries.end();
}

bool Metadata::getBool(String key) const
{
	return getString(key) == "true";
}

int Metadata::getInt(String key) const
{
	return getString(key).toInteger();
}

float Metadata::getFloat(String key) const
{
	return getString(key).toFloat();
}

String Metadata::getString(String key) const
{
	auto result = entries.find(key);
	if (result != entries.end()) {
		return result->second;
	} else {
		throw Exception("Key " + key + " not found in metafile.");
	}
}

bool Metadata::getBool(String key, bool v) const
{
	if (hasKey(key)) {
		return getBool(key);
	} else {
		return v;
	}
}

int Metadata::getInt(String key, int v) const
{
	if (hasKey(key)) {
		return getInt(key);
	} else {
		return v;
	}
}

float Metadata::getFloat(String key, float v) const
{
	if (hasKey(key)) {
		return getFloat(key);
	} else {
		return v;
	}
}

String Metadata::getString(String key, String v) const
{
	if (hasKey(key)) {
		return getString(key);
	} else {
		return v;
	}
}

void Metadata::set(String key, bool value)
{
	entries[key] = value ? "true" : "false";
}

void Metadata::set(String key, int value)
{
	entries[key] = toString(value);
}

void Metadata::set(String key, float value)
{
	entries[key] = toString(value);
}

void Metadata::set(String key, const char* value)
{
	entries[key] = value;
}

void Metadata::set(String key, const std::string& value)
{
	entries[key] = value;
}

void Metadata::set(String key, const String& value)
{
	entries[key] = value;
}

std::unique_ptr<Metadata> Metadata::fromBinary(ResourceDataStatic& data)
{
	auto meta = std::make_unique<Metadata>();
	Deserializer s(data.getSpan());
	meta->deserialize(s);
	return std::move(meta);
}

void Metadata::serialize(Serializer& s) const
{
	s << entries;
}

void Metadata::deserialize(Deserializer& s)
{
	s >> entries;
}

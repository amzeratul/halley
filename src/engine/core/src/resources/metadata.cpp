#include "halley/resources/metadata.h"
#include "halley/resources/resource_data.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/text/string_converter.h"
#include "halley/file_formats/config_file.h"

using namespace Halley;

Metadata::Metadata()
	: entries(ConfigNode::MapType())
{
}

Metadata::~Metadata() {}

bool Metadata::hasKey(std::string_view key) const
{
	return entries.hasKey(key);
}

bool Metadata::getBool(std::string_view key) const
{
	return entries[key].asBool();
}

int Metadata::getInt(std::string_view key) const
{
	return entries[key].asInt();
}

float Metadata::getFloat(std::string_view key) const
{
	return entries[key].asFloat();
}

String Metadata::getString(std::string_view key) const
{
	return entries[key].asString();
}

ConfigNode Metadata::getValue(std::string_view key) const
{
	return ConfigNode(entries[key]);
}

bool Metadata::getBool(std::string_view key, bool v) const
{
	return entries[key].asBool(v);
}

int Metadata::getInt(std::string_view key, int v) const
{
	return entries[key].asInt(v);
}

float Metadata::getFloat(std::string_view key, float v) const
{
	return entries[key].asFloat(v);
}

String Metadata::getString(std::string_view key, String v) const
{
	return entries[key].asString(v);
}

const ConfigNode& Metadata::getEntries() const
{
	return entries;
}

bool Metadata::set(std::string_view key, std::string_view value)
{
	auto& es = entries.asMap();
	const auto iter = es.find(key);
	if (iter != es.end()) {
		// Value exists
		if (value.empty()) {
			erase(key);
			return true;
		}

		auto v = ConfigNode(value);
		if (iter->second == v) {
			return false;
		}
		iter->second = std::move(v);
	} else {
		// Value didn't exist
		if (value.empty()) {
			return false;
		}

		es[key] = ConfigNode(value);
	}
	return true;
}

bool Metadata::set(std::string_view key, const char* value)
{
	return set(key, std::string_view(value));
}

bool Metadata::set(std::string_view key, const std::string& value)
{
	return set(key, std::string_view(value));
}

bool Metadata::set(std::string_view key, const String& value)
{
	return set(key, std::string_view(value));
}

bool Metadata::erase(std::string_view key)
{
	return entries.removeKey(key);
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
	return entries.asString();
}

ConfigNode Metadata::toConfig() const
{
	return ConfigNode(entries);
}

size_t Metadata::getMemoryUsage() const
{
	return entries.getSizeBytes();
}

#include "halley/file_formats/config_file.h"
#include "halley/file/byte_serializer.h"
#include "halley/support/exception.h"
#include "halley/core/resources/resource_collection.h"

using namespace Halley;

ConfigNode::ConfigNode()
{
}

ConfigNode::ConfigNode(const ConfigNode& other)
{
	*this = other;
}

ConfigNode::ConfigNode(ConfigNode&& other)
{
	*this = std::move(other);
}

ConfigNode::ConfigNode(MapType&& entryMap)
{
	operator=(std::move(entryMap));
}

ConfigNode::ConfigNode(SequenceType&& entryList)
{
	operator=(std::move(entryList));
}

ConfigNode::ConfigNode(String&& value)
{
	operator=(std::move(value));
}

ConfigNode::ConfigNode(bool value)
{
	operator=(value);
}

ConfigNode::ConfigNode(int value)
{
	operator=(value);
}

ConfigNode::ConfigNode(float value)
{
	operator=(value);
}

ConfigNode::ConfigNode(Vector2i value)
{
	operator=(value);
}

ConfigNode::ConfigNode(Vector2f value)
{
	operator=(value);
}

ConfigNode::ConfigNode(Bytes&& value)
{
	operator=(std::move(value));
}

void ConfigNode::removeKey(const String& key)
{
	asMap().erase(key);
}

ConfigNode::~ConfigNode()
{
	reset();
}

ConfigNode& ConfigNode::operator=(ConfigNode&& other)
{
	type = other.type;
	ptrData = other.ptrData;
	intData = other.intData;
	floatData = other.floatData;
	vec2iData = other.vec2iData;
	vec2fData = other.vec2fData;
	other.type = ConfigNodeType::Undefined;
	other.ptrData = nullptr;
	return *this;
}

ConfigNode& ConfigNode::operator=(bool value)
{
	reset();
	type = ConfigNodeType::Int;
	intData = value ? 1 : 0;
	return *this;
}

ConfigNode& ConfigNode::operator=(int value)
{
	reset();
	type = ConfigNodeType::Int;
	intData = value;
	return *this;
}

ConfigNode& ConfigNode::operator=(float value)
{
	reset();
	type = ConfigNodeType::Float;
	floatData = value;
	return *this;
}

ConfigNode& ConfigNode::operator=(Vector2i value)
{
	reset();
	type = ConfigNodeType::Int2;
	vec2iData = value;
	return *this;
}

ConfigNode& ConfigNode::operator=(Vector2f value)
{
	reset();
	type = ConfigNodeType::Float2;
	vec2fData = value;
	return *this;
}

ConfigNode& ConfigNode::operator=(const Bytes& value)
{
	reset();
	type = ConfigNodeType::Bytes;
	ptrData = new Bytes(value);
	return *this;
}

ConfigNode& ConfigNode::operator=(Bytes&& value)
{
	reset();
	type = ConfigNodeType::Bytes;
	ptrData = new Bytes(std::move(value));
	return *this;
}

ConfigNode& ConfigNode::operator=(const ConfigNode& other)
{
	switch (other.type) {
		case ConfigNodeType::String:
			*this = other.asString();
			break;
		case ConfigNodeType::Sequence:
			*this = other.asSequence();
			break;
		case ConfigNodeType::Map:
			*this = other.asMap();
			break;
		case ConfigNodeType::Int:
			*this = other.asInt();
			break;
		case ConfigNodeType::Float:
			*this = other.asFloat();
			break;
		case ConfigNodeType::Int2:
			*this = other.asVector2i();
			break;
		case ConfigNodeType::Float2:
			*this = other.asVector2f();
			break;
		case ConfigNodeType::Bytes:
			*this = other.asBytes();
			break;
		case ConfigNodeType::Undefined:
			break;
		default:
			throw Exception("Unknown configuration node type.");
	}
	return *this;
}

ConfigNode& ConfigNode::operator=(const MapType& entry)
{
	reset();
	type = ConfigNodeType::Map;
	ptrData = new MapType(entry);
	return *this;
}

ConfigNode& ConfigNode::operator=(MapType&& entry) 
{
	reset();
	type = ConfigNodeType::Map;
	ptrData = new MapType(std::move(entry));
	return *this;
}

ConfigNode& ConfigNode::operator=(const SequenceType& entry)
{
	reset();
	type = ConfigNodeType::Sequence;
	ptrData = new SequenceType(entry);
	return *this;
}

ConfigNode& ConfigNode::operator=(SequenceType&& entry) 
{
	reset();
	type = ConfigNodeType::Sequence;
	ptrData = new SequenceType(std::move(entry));
	return *this;
}

ConfigNode& ConfigNode::operator=(const String& entry)
{
	reset();
	type = ConfigNodeType::String;
	ptrData = new String(entry);
	return *this;
}

ConfigNode& ConfigNode::operator=(String&& entry) 
{
	reset();
	type = ConfigNodeType::String;
	ptrData = new String(std::move(entry));
	return *this;
}

ConfigNodeType ConfigNode::getType() const
{
	return type;
}

void ConfigNode::serialize(Serializer& s) const
{
	s << type;

	switch (type) {
		case ConfigNodeType::String:
		{
			s << asString();
			break;
		}
		case ConfigNodeType::Sequence:
		{
			s << asSequence();
			break;
		}
		case ConfigNodeType::Map:
		{
			s << asMap();
			break;
		}
		case ConfigNodeType::Int:
		{
			s << asInt();
			break;
		}
		case ConfigNodeType::Float:
		{
			s << asFloat();
			break;
		}
		case ConfigNodeType::Int2:
		{
			s << asVector2i();
			break;
		}
		case ConfigNodeType::Float2:
		{
			s << asVector2f();
			break;
		}
		case ConfigNodeType::Bytes:
		{
			s << asBytes();
			break;
		}
		case ConfigNodeType::Undefined:
		{
			break;
		}
		default:
			throw Exception("Unknown configuration node type.");
	}
}

void ConfigNode::deserialize(Deserializer& s)
{
	ConfigNodeType incomingType;
	s >> incomingType;

	switch (incomingType) {
		case ConfigNodeType::String:
		{
			deserializeContents<String>(s);
			break;
		}
		case ConfigNodeType::Sequence:
		{
			deserializeContents<SequenceType>(s);
			break;
		}
		case ConfigNodeType::Map:
		{
			deserializeContents<MapType>(s);
			break;
		}
		case ConfigNodeType::Int:
		{
			deserializeContents<int>(s);
			break;
		}
		case ConfigNodeType::Float:
		{
			deserializeContents<float>(s);
			break;
		}
		case ConfigNodeType::Int2:
		{
			deserializeContents<Vector2i>(s);
			break;
		}
		case ConfigNodeType::Float2:
		{
			deserializeContents<Vector2f>(s);
			break;
		}
		case ConfigNodeType::Bytes:
		{
			deserializeContents<Bytes>(s);
			break;
		}
		case ConfigNodeType::Undefined:
		{
			break;
		}
		default:
			throw Exception("Unknown configuration node type.");
	}
}

int ConfigNode::asInt() const
{
	if (type == ConfigNodeType::Int) {
		return intData;
	} else if (type == ConfigNodeType::Float) {
		return int(floatData);
	} else if (type == ConfigNodeType::String) {
		return asString().toInteger();
	} else {
		throw Exception("Not cannot be converted to int.");
	}
}

float ConfigNode::asFloat() const
{
	if (type == ConfigNodeType::Int) {
		return float(intData);
	} else if (type == ConfigNodeType::Float) {
		return floatData;
	} else if (type == ConfigNodeType::String) {
		return asString().toFloat();
	} else {
		throw Exception("Not cannot be converted to float.");
	}
}

bool ConfigNode::asBool() const
{
	if (type == ConfigNodeType::Int) {
		return intData != 0;
	} else {
		return asString() == "true";
	}
}

Vector2i ConfigNode::asVector2i() const
{
	if (type == ConfigNodeType::Int2) {
		return vec2iData;
	} else if (type == ConfigNodeType::Float2) {
		return Vector2i(vec2fData);
	} else {
		throw Exception("Node is not a vector type");
	}
}

Vector2f ConfigNode::asVector2f() const
{
	if (type == ConfigNodeType::Int2) {
		return Vector2f(vec2iData);
	} else if (type == ConfigNodeType::Float2) {
		return vec2fData;
	} else {
		throw Exception("Node is not a vector type");
	}
}

const Bytes& ConfigNode::asBytes() const
{
	if (type == ConfigNodeType::Bytes) {
		return *reinterpret_cast<Bytes*>(ptrData);
	} else {
		throw Exception("Node is not a byte sequence type");
	}
}

Vector2i ConfigNode::asVector2i(Vector2i defaultValue) const
{
	if (type == ConfigNodeType::Undefined) {
		return defaultValue;
	} else {
		return asVector2i();
	}
}

Vector2f ConfigNode::asVector2f(Vector2f defaultValue) const
{
	if (type == ConfigNodeType::Undefined) {
		return defaultValue;
	} else {
		return asVector2f();
	}
}

String ConfigNode::asString() const
{
	if (type == ConfigNodeType::String) {
		return *reinterpret_cast<String*>(ptrData);
	} else {
		throw Exception("Node is not a string type");
	}
}

int ConfigNode::asInt(int defaultValue) const
{
	if (type == ConfigNodeType::Undefined) {
		return defaultValue;
	} else {
		return asInt();
	}
}

float ConfigNode::asFloat(float defaultValue) const
{
	if (type == ConfigNodeType::Undefined) {
		return defaultValue;
	} else {
		return asFloat();
	}
}

bool ConfigNode::asBool(bool defaultValue) const
{
	if (type == ConfigNodeType::Undefined) {
		return defaultValue;
	} else {
		return asBool();
	}
}

String ConfigNode::asString(const String& defaultValue) const
{
	if (type == ConfigNodeType::Undefined) {
		return defaultValue;
	} else {
		return asString();
	}
}

const ConfigNode::SequenceType& ConfigNode::asSequence() const
{
	if (type == ConfigNodeType::Sequence) {
		return *reinterpret_cast<SequenceType*>(ptrData);
	} else {
		throw Exception("Node is not a sequence type");
	}
}

const ConfigNode::MapType& ConfigNode::asMap() const
{
	if (type == ConfigNodeType::Map) {
		return *reinterpret_cast<MapType*>(ptrData);
	} else {
		throw Exception("Node is not a map type");
	}
}

ConfigNode::SequenceType& ConfigNode::asSequence()
{
	if (type == ConfigNodeType::Sequence) {
		return *reinterpret_cast<SequenceType*>(ptrData);
	} else {
		throw Exception("Node is not a sequence type");
	}
}

ConfigNode::MapType& ConfigNode::asMap()
{
	if (type == ConfigNodeType::Map) {
		return *reinterpret_cast<MapType*>(ptrData);
	} else {
		throw Exception("Node is not a map type");
	}
}

bool ConfigNode::hasKey(const String& key) const
{
	if (type == ConfigNodeType::Map) {
		auto& map = asMap();
		return map.find(key) != map.end();
	} else {
		return false;
	}
}

ConfigNode& ConfigNode::operator[](const String& key)
{
	return asMap()[key];
}

ConfigNode& ConfigNode::operator[](size_t idx)
{
	return asSequence().at(idx);
}

const ConfigNode& ConfigNode::operator[](const String& key) const
{
	auto& map = asMap();
	auto iter = map.find(key);
	if (iter != map.end()) {
		return iter->second;
	} else {
		return undefinedConfigNode;
	}
}

const ConfigNode& ConfigNode::operator[](size_t idx) const
{
	return asSequence().at(idx);
}

std::vector<ConfigNode>::iterator ConfigNode::begin()
{
	return asSequence().begin();
}

std::vector<ConfigNode>::iterator ConfigNode::end()
{
	return asSequence().end();
}

std::vector<ConfigNode>::const_iterator ConfigNode::begin() const
{
	return asSequence().begin();
}

std::vector<ConfigNode>::const_iterator ConfigNode::end() const
{
	return asSequence().end();
}

void ConfigNode::reset()
{
	if (type == ConfigNodeType::Map) {
		delete reinterpret_cast<MapType*>(ptrData);
	} else if (type == ConfigNodeType::Sequence) {
		delete reinterpret_cast<SequenceType*>(ptrData);
	} else if (type == ConfigNodeType::Bytes) {
		delete reinterpret_cast<Bytes*>(ptrData);
	} else if (type == ConfigNodeType::String) {
		delete reinterpret_cast<String*>(ptrData);
	}
	ptrData = nullptr;
	type = ConfigNodeType::Undefined;
}

ConfigNode& ConfigFile::getRoot()
{
	return root;
}

const ConfigNode& ConfigFile::getRoot() const
{
	return root;
}

constexpr int curVersion = 1;

void ConfigFile::serialize(Serializer& s) const
{
	int version = curVersion;
	s << version;
	s << root;
}

void ConfigFile::deserialize(Deserializer& s)
{
	int version;
	s >> version;
	s >> root;
}

std::unique_ptr<ConfigFile> ConfigFile::loadResource(ResourceLoader& loader)
{
	auto config = std::make_unique<ConfigFile>();

	auto data = loader.getStatic();
	Deserializer s(data->getSpan());
	s >> *config;

	return config;
}

ConfigNode ConfigNode::undefinedConfigNode;

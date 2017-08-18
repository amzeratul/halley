#include "halley/file_formats/config_file.h"
#include "halley/file/byte_serializer.h"
#include "halley/support/exception.h"
#include "halley/core/resources/resource_collection.h"

using namespace Halley;

ConfigNode::ConfigNode()
	: type(ConfigNodeType::Undefined)
	, contents("")
{
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

ConfigNode& ConfigNode::operator=(int value)
{
	type = ConfigNodeType::Int;
	contents = value;
	return *this;
}

ConfigNode& ConfigNode::operator=(float value)
{
	type = ConfigNodeType::Float;
	contents = value;
	return *this;
}

ConfigNode& ConfigNode::operator=(Vector2i value)
{
	type = ConfigNodeType::Int2;
	contents = value;
	return *this;
}

ConfigNode& ConfigNode::operator=(Vector2f value)
{
	type = ConfigNodeType::Float2;
	contents = value;
	return *this;
}

ConfigNode& ConfigNode::operator=(Bytes&& value)
{
	type = ConfigNodeType::Bytes;
	contents = std::move(value);
	return *this;
}

ConfigNode& ConfigNode::operator=(const Bytes& value)
{
	type = ConfigNodeType::Bytes;
	contents = std::move(value);
	return *this;
}

ConfigNode& ConfigNode::operator=(MapType&& entry) 
{
	type = ConfigNodeType::Map;
	contents = std::move(entry);
	return *this;
}

ConfigNode& ConfigNode::operator=(SequenceType&& entry) 
{
	type = ConfigNodeType::Sequence;
	contents = std::move(entry);
	return *this;
}

ConfigNode& ConfigNode::operator=(String&& entry) 
{
	type = ConfigNodeType::String;
	contents = std::move(entry);
	return *this;
}

ConfigNode& ConfigNode::operator=(const MapType& entry)
{
	type = ConfigNodeType::Map;
	contents = entry;
	return *this;
}

ConfigNode& ConfigNode::operator=(const SequenceType& entry)
{
	type = ConfigNodeType::Sequence;
	contents = entry;
	return *this;
}

ConfigNode& ConfigNode::operator=(const String& entry)
{
	type = ConfigNodeType::String;
	contents = entry;
	return *this;
}

ConfigNodeType ConfigNode::getType() const
{
	return type;
}

class SerializerVisitor : public boost::static_visitor<>
{
public:
	SerializerVisitor(Serializer& s) : serializer(s) {}
	template <typename T> void operator() (T& v) const { serializer << v; }
private:
	Serializer& serializer;
};

void ConfigNode::serialize(Serializer& s) const
{
	s << type;
	boost::apply_visitor(SerializerVisitor(s), contents);
}

void ConfigNode::deserialize(Deserializer& s)
{
	s >> type;

	switch (type) {
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
		return boost::get<int>(contents);
	} else if (type == ConfigNodeType::Float) {
		return int(boost::get<float>(contents));
	} else if (type == ConfigNodeType::String) {
		return asString().toInteger();
	} else {
		throw Exception("Not cannot be converted to int.");
	}
}

float ConfigNode::asFloat() const
{
	if (type == ConfigNodeType::Int) {
		return float(boost::get<int>(contents));
	} else if (type == ConfigNodeType::Float) {
		return boost::get<float>(contents);
	} else if (type == ConfigNodeType::String) {
		return asString().toFloat();
	} else {
		throw Exception("Not cannot be converted to float.");
	}}

bool ConfigNode::asBool() const
{
	return asString() == "true";
}

Vector2i ConfigNode::asVector2i() const
{
	if (type == ConfigNodeType::Int2) {
		return boost::get<Vector2i>(contents);
	} else if (type == ConfigNodeType::Float2) {
		return Vector2i(boost::get<Vector2f>(contents));
	} else {
		throw Exception("Node is not a vector type");
	}
}

Vector2f ConfigNode::asVector2f() const
{
	if (type == ConfigNodeType::Int2) {
		return Vector2f(boost::get<Vector2i>(contents));
	} else if (type == ConfigNodeType::Float2) {
		return boost::get<Vector2f>(contents);
	} else {
		throw Exception("Node is not a vector type");
	}
}

const Bytes& ConfigNode::asBytes() const
{
	if (type == ConfigNodeType::Bytes) {
		return boost::get<Bytes>(contents);
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
		return boost::get<String>(contents);
	} else {
		throw Exception("Node is not a scalar type");
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
		return boost::get<SequenceType>(contents);
	} else {
		throw Exception("Node is not a scalar type");
	}
}

const ConfigNode::MapType& ConfigNode::asMap() const
{
	if (type == ConfigNodeType::Map) {
		return boost::get<MapType>(contents);
	} else {
		throw Exception("Node is not a scalar type");
	}
}

ConfigNode::SequenceType& ConfigNode::asSequence()
{
	if (type == ConfigNodeType::Sequence) {
		return boost::get<SequenceType>(contents);
	} else {
		throw Exception("Node is not a sequence type");
	}
}

ConfigNode::MapType& ConfigNode::asMap()
{
	if (type == ConfigNodeType::Map) {
		return boost::get<MapType>(contents);
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

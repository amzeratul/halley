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

ConfigNode& ConfigNode::operator=(ScalarType&& entry) 
{
	type = ConfigNodeType::Scalar;
	contents = std::move(entry);
	return *this;
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
	s << int(type);
	boost::apply_visitor(SerializerVisitor(s), contents);
}

void ConfigNode::deserialize(Deserializer& s)
{
	int temp;
	s >> temp;
	type = ConfigNodeType(temp);

	switch (type) {
		case ConfigNodeType::Scalar:
		{
			ScalarType scalar;
			s >> scalar;
			contents = scalar;
			break;
		}
		case ConfigNodeType::Sequence:
		{
			SequenceType scalar;
			s >> scalar;
			contents = scalar;
			break;
		}
		case ConfigNodeType::Map:
		{
			MapType scalar;
			s >> scalar;
			contents = scalar;
			break;
		}
		default:
			throw Exception("Unknown configuration node type.");
	}
}

int ConfigNode::asInt() const
{
	return asString().toInteger();
}

float ConfigNode::asFloat() const
{
	return asString().toFloat();
}

bool ConfigNode::asBool() const
{
	return asString() == "true";
}

String ConfigNode::asString() const
{
	if (type == ConfigNodeType::Scalar) {
		return boost::get<ScalarType>(contents);
	} else {
		throw Exception("Node is not a scalar type");
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
		throw Exception("Key " + key + " not found in map.");
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

void ConfigFile::serialize(Serializer& s) const
{
	s << root;
}

void ConfigFile::deserialize(Deserializer& s)
{
	s >> root;
}

std::unique_ptr<ConfigFile> ConfigFile::loadResource(ResourceLoader& loader)
{
	auto config = std::make_unique<ConfigFile>();

	auto data = loader.getStatic();
	Deserializer s(data->getSpan());
	s >> *config;

	return std::move(config);
}

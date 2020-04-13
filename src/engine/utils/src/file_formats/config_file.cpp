#include "halley/file_formats/config_file.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/support/exception.h"
#include "halley/core/resources/resource_collection.h"

using namespace Halley;

ConfigNode::ConfigNode()
{
}

ConfigNode::ConfigNode(const ConfigNode& other)
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
			throw Exception("Unknown configuration node type.", HalleyExceptions::Resources);
	}
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
	reset();

	type = other.type;
	ptrData = other.ptrData;
	intData = other.intData;
	floatData = other.floatData;
	vec2iData = other.vec2iData;
	vec2fData = other.vec2fData;
	line = other.line;
	column = other.column;
	parent = other.parent;
	parentIdx = other.parentIdx;
	parentFile = other.parentFile;
	
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

ConfigNode& ConfigNode::operator=(const char* value)
{
	*this = String(value);
	return *this;
}

ConfigNode& ConfigNode::operator=(Bytes&& value)
{
	reset();
	type = ConfigNodeType::Bytes;
	ptrData = new Bytes(std::move(value));
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
			throw Exception("Unknown configuration node type.", HalleyExceptions::Resources);
	}

	s << line;
	s << column;
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
			throw Exception("Unknown configuration node type.", HalleyExceptions::Resources);
	}

	if (s.getVersion() >= 2) {
		s >> line;
		s >> column;
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
		throw Exception(getNodeDebugId() + " cannot be converted to int.", HalleyExceptions::Resources);
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
		throw Exception(getNodeDebugId() + " cannot be converted to float.", HalleyExceptions::Resources);
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
	} else if (type == ConfigNodeType::Sequence) {
		auto& seq = asSequence();
		return Vector2i(seq.at(0).asInt(), seq.at(1).asInt());
	} else {
		throw Exception(getNodeDebugId() + " is not a vector type", HalleyExceptions::Resources);
	}
}

Vector2f ConfigNode::asVector2f() const
{
	if (type == ConfigNodeType::Int2) {
		return Vector2f(vec2iData);
	} else if (type == ConfigNodeType::Float2) {
		return vec2fData;
	} else if (type == ConfigNodeType::Sequence) {
		auto& seq = asSequence();
		return Vector2f(seq.at(0).asFloat(), seq.at(1).asFloat());
	} else {
		throw Exception(getNodeDebugId() + " is not a vector type", HalleyExceptions::Resources);
	}
}

const Bytes& ConfigNode::asBytes() const
{
	if (type == ConfigNodeType::Bytes) {
		return *reinterpret_cast<Bytes*>(ptrData);
	} else {
		throw Exception(getNodeDebugId() + " is not a byte sequence type", HalleyExceptions::Resources);
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
	} else if (type == ConfigNodeType::Int) {
		return toString(asInt());
	} else if (type == ConfigNodeType::Float) {
		return toString(asFloat());
	} else if (type == ConfigNodeType::Sequence) {
		String result = "[";
		bool first = true;
		for (auto& e: asSequence()) {
			if (!first) {
				result += ", ";
			}
			first = false;
			result += e.asString();
		}
		result += "]";
		return result;
	} else if (type == ConfigNodeType::Float2) {
		auto v = asVector2f();
		return "(" + toString(v.x) + ", " + toString(v.y) + ")";
	} else if (type == ConfigNodeType::Int2) {
		auto v = asVector2i();
		return "(" + toString(v.x) + ", " + toString(v.y) + ")";
	} else if (type == ConfigNodeType::Map) {
		return "{...}";
	} else {
		throw Exception("Can't convert " + getNodeDebugId() + " from " + toString(getType()) + " to String.", HalleyExceptions::Resources);
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
		throw Exception(getNodeDebugId() + " is not a sequence type", HalleyExceptions::Resources);
	}
}

const ConfigNode::MapType& ConfigNode::asMap() const
{
	if (type == ConfigNodeType::Map) {
		return *reinterpret_cast<MapType*>(ptrData);
	} else {
		throw Exception(getNodeDebugId() + " is not a map type", HalleyExceptions::Resources);
	}
}

ConfigNode::SequenceType& ConfigNode::asSequence()
{
	if (type == ConfigNodeType::Sequence) {
		return *reinterpret_cast<SequenceType*>(ptrData);
	} else {
		throw Exception(getNodeDebugId() + " is not a sequence type", HalleyExceptions::Resources);
	}
}

ConfigNode::MapType& ConfigNode::asMap()
{
	if (type == ConfigNodeType::Map) {
		return *reinterpret_cast<MapType*>(ptrData);
	} else {
		throw Exception(getNodeDebugId() + " is not a map type", HalleyExceptions::Resources);
	}
}

bool ConfigNode::hasKey(const String& key) const
{
	if (type == ConfigNodeType::Map) {
		auto& map = asMap();
		auto iter = map.find(key);
		return iter != map.end() && iter->second.getType() != ConfigNodeType::Undefined;
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
		// WARNING: NOT THREAD SAFE
		undefinedConfigNode.setParent(this, -1);
		undefinedConfigNode.parentFile = parentFile;
		undefinedConfigNodeName = key;
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

void ConfigNode::setOriginalPosition(int l, int c)
{
	line = l;
	column = c;
}

void ConfigNode::setParent(const ConfigNode* p, int idx)
{
	parent = p;
	parentIdx = idx;
}

void ConfigNode::propagateParentingInformation(const ConfigFile* file)
{
	parentFile = file;
	if (type == ConfigNodeType::Sequence) {
		int i = 0;
		for (auto& e: asSequence()) {
			e.setParent(this, i++);
			e.propagateParentingInformation(file);
		}
	} else if (type == ConfigNodeType::Map) {
		int i = 0;
		for (auto& e: asMap()) {
			e.second.setParent(this, i++);
			e.second.propagateParentingInformation(file);
		}
	}
}

String ConfigNode::getNodeDebugId() const
{
	String value;
	switch (type) {
		case ConfigNodeType::String:
			value = "\"" + asString() + "\"";
			break;
		case ConfigNodeType::Sequence:
			value = "Sequence[" + toString(asSequence().size()) + "]";
			break;
		case ConfigNodeType::Map:
			value = "Map";
			break;
		case ConfigNodeType::Int:
			value = toString(asInt());
			break;
		case ConfigNodeType::Float:
			value = toString(asFloat());
			break;
		case ConfigNodeType::Int2:
			{
				auto v = asVector2i();
				value = "Vector2i(" + toString(v.x) + ", " + toString(v.y) + ")";
			}
			break;
		case ConfigNodeType::Float2:
			{
				auto v = asVector2f();
				value = "Vector2f(" + toString(v.x) + ", " + toString(v.y) + ")";
			}
			break;
		case ConfigNodeType::Bytes:
			value = "Bytes (" + String::prettySize(asBytes().size()) + ")";
			break;
		case ConfigNodeType::Undefined:
			value = "null";
			break;
	}

	String assetId = "unknown";
	if (parentFile) {
		assetId = parentFile->getAssetId();
	}
	return "Node \"" + backTrackFullNodeName() + "\" (" + value + ") at \"" + assetId + "(" + toString(line + 1) + ":" + toString(column + 1) + ")\"";
}

String ConfigNode::backTrackFullNodeName() const
{
	if (parent) {
		if (parent->type == ConfigNodeType::Sequence) {
			return parent->backTrackFullNodeName() + "[" + toString(parentIdx) + "]";
		} else if (parent->type == ConfigNodeType::Map) {
			auto& parentMap = parent->asMap();
			int i = 0;
			String name = "?";
			if (parentIdx == -1) {
				name = ConfigNode::undefinedConfigNodeName;
			} else {
				for (auto& e: parentMap) {
					if (i++ == parentIdx) {
						name = e.first;
						break;
					}
				}
			}
			return parent->backTrackFullNodeName() + "." + name;
		} else {
			return "?";
		}
	} else {
		return "~";
	}
}

ConfigFile::ConfigFile()
{
}

ConfigFile::ConfigFile(const ConfigFile& other)
{
	root = ConfigNode(other.root);
	updateRoot();
}

ConfigFile::ConfigFile(ConfigFile&& other)
{
	root = std::move(other.root);
	updateRoot();
}

ConfigFile& ConfigFile::operator=(ConfigFile&& other)
{
	root = std::move(other.root);
	updateRoot();
	return *this;
}

ConfigNode& ConfigFile::getRoot()
{
	return root;
}

const ConfigNode& ConfigFile::getRoot() const
{
	return root;
}

constexpr int curVersion = 2;

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
	s.setVersion(version);
	s >> root;

	updateRoot();
}

std::unique_ptr<ConfigFile> ConfigFile::loadResource(ResourceLoader& loader)
{
	auto config = std::make_unique<ConfigFile>();

	auto data = loader.getStatic();
	Deserializer s(data->getSpan());
	s >> *config;

	return config;
}

void ConfigFile::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<ConfigFile&>(resource));
	updateRoot();
}

void ConfigFile::updateRoot()
{
	root.propagateParentingInformation(this);
	Ensures(root.parentIdx == 0);
	Ensures(root.parent == nullptr);
	Ensures(root.parentFile == this);
}

ConfigObserver::ConfigObserver()
{
}

ConfigObserver::ConfigObserver(const ConfigNode& node)
	: node(&node)
{
}

ConfigObserver::ConfigObserver(const ConfigFile& file)
	: file(&file)
	, node(&file.getRoot())
{
}

const ConfigNode& ConfigObserver::getRoot() const
{
	Expects(node);
	return *node;
}

bool ConfigObserver::needsUpdate() const
{
	return file && assetVersion != file->getAssetVersion();
}

void ConfigObserver::update()
{
	if (file) {
		assetVersion = file->getAssetVersion();
		node = &file->getRoot();
	}
}

String ConfigObserver::getAssetId() const
{
	if (file) {
		return file->getAssetId();
	} else {
		return "";
	}
}

std::unique_ptr<Prefab> Prefab::loadResource(ResourceLoader& loader)
{
	auto prefab = std::make_unique<Prefab>();

	auto data = loader.getStatic();
	Deserializer::fromBytes(*prefab, data->getSpan());

	return prefab;
}

void Prefab::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<Prefab&>(resource));
	updateRoot();
}

std::unique_ptr<Scene> Scene::loadResource(ResourceLoader& loader)
{
	auto scene = std::make_unique<Scene>();
	auto data = loader.getStatic();
	Deserializer::fromBytes(*scene, data->getSpan());

	return scene;
}

void Scene::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<Scene&>(resource));
	updateRoot();
}

ConfigNode ConfigNode::undefinedConfigNode;
String ConfigNode::undefinedConfigNodeName;

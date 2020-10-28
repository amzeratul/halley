#include "halley/data_structures/config_node.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/file_formats/config_file.h"
#include "halley/support/exception.h"
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

ConfigNode::ConfigNode(ConfigNode&& other) noexcept
{
	*this = std::move(other);
}

ConfigNode::ConfigNode(MapType entryMap)
{
	operator=(std::move(entryMap));
}

ConfigNode::ConfigNode(SequenceType entryList)
{
	operator=(std::move(entryList));
}

ConfigNode::ConfigNode(String value)
{
	operator=(std::move(value));
}

ConfigNode::ConfigNode(const char* value)
{
	operator=(value);
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

ConfigNode::ConfigNode(Bytes value)
{
	operator=(std::move(value));
}

ConfigNode::ConfigNode(NoopType value)
{
	operator=(value);
}

ConfigNode::ConfigNode(DelType value)
{
	operator=(value);
}

ConfigNode::ConfigNode(IdxType value)
{
	operator=(value);
}

void ConfigNode::removeKey(const String& key)
{
	asMap().erase(key);
}

ConfigNode::~ConfigNode()
{
	reset();
}

ConfigNode& ConfigNode::operator=(ConfigNode&& other) noexcept
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

ConfigNode& ConfigNode::operator=(const char* value)
{
	*this = String(value);
	return *this;
}

bool ConfigNode::operator==(const ConfigNode& other) const
{
	if (type != other.type) {
		return false;
	}
	
	switch (type) {
		case ConfigNodeType::String:
			return asString() == other.asString();
		case ConfigNodeType::Sequence:
			return asSequence() == other.asSequence();
		case ConfigNodeType::Map:
			return asMap() == other.asMap();
		case ConfigNodeType::Int:
			return asInt() == other.asInt();
		case ConfigNodeType::Float:
			return asFloat() == other.asFloat();
		case ConfigNodeType::Int2:
		case ConfigNodeType::Idx:
			return asVector2i() == other.asVector2i();
		case ConfigNodeType::Float2:
			return asVector2f() == other.asVector2f();
		case ConfigNodeType::Bytes:
			return asBytes() == other.asBytes();
		default:
			return true;
	}
}

bool ConfigNode::operator!=(const ConfigNode& other) const
{
	return !(*this == other);
}

ConfigNode& ConfigNode::operator=(Bytes value)
{
	reset();
	type = ConfigNodeType::Bytes;
	ptrData = new Bytes(std::move(value));
	return *this;
}

ConfigNode& ConfigNode::operator=(gsl::span<const gsl::byte> bytes)
{
	reset();
	type = ConfigNodeType::Bytes;
	auto b = new Bytes(bytes.size_bytes());
	memcpy(b->data(), bytes.data(), bytes.size_bytes());
	ptrData = b;
	return *this;
}

ConfigNode& ConfigNode::operator=(MapType entry) 
{
	reset();
	type = ConfigNodeType::Map;
	ptrData = new MapType(std::move(entry));
	return *this;
}

ConfigNode& ConfigNode::operator=(SequenceType entry) 
{
	reset();
	type = ConfigNodeType::Sequence;
	ptrData = new SequenceType(std::move(entry));
	return *this;
}

ConfigNode& ConfigNode::operator=(String entry) 
{
	reset();
	type = ConfigNodeType::String;
	ptrData = new String(std::move(entry));
	return *this;
}

ConfigNode& ConfigNode::operator=(NoopType value)
{
	reset();
	type = ConfigNodeType::Noop;
	return *this;
}

ConfigNode& ConfigNode::operator=(DelType value)
{
	reset();
	type = ConfigNodeType::Del;
	return *this;
}

ConfigNode& ConfigNode::operator=(IdxType value)
{
	reset();
	type = ConfigNodeType::Idx;
	vec2iData = Vector2i(value.start, value.len);
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
		case ConfigNodeType::Idx:
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
		case ConfigNodeType::Del:
		case ConfigNodeType::Noop:
		{
			break;
		}
		default:
			throw Exception("Unknown configuration node type.", HalleyExceptions::Resources);
	}

	const auto state = s.getState<ConfigFileSerializationState>();
	if (state && state->storeFilePosition) {
		s << line;
		s << column;
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
		case ConfigNodeType::Idx:
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
		case ConfigNodeType::Noop:
		case ConfigNodeType::Del:
		case ConfigNodeType::Undefined:
		{
			break;
		}
		default:
			throw Exception("Unknown configuration node type.", HalleyExceptions::Resources);
	}

	const auto state = s.getState<ConfigFileSerializationState>();
	if (state && state->storeFilePosition) {
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
		throw Exception(getNodeDebugId() + " is not a vector2 type", HalleyExceptions::Resources);
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
		throw Exception(getNodeDebugId() + " is not a vector2 type", HalleyExceptions::Resources);
	}
}

Vector4i ConfigNode::asVector4i() const
{
	if (type == ConfigNodeType::Sequence) {
		auto& seq = asSequence();
		return Vector4i(seq.at(0).asInt(), seq.at(1).asInt(), seq.at(2).asInt(), seq.at(3).asInt());
	} else {
		throw Exception(getNodeDebugId() + " is not a vector4 type", HalleyExceptions::Resources);
	}
}

Vector4f ConfigNode::asVector4f() const
{
	if (type == ConfigNodeType::Sequence) {
		auto& seq = asSequence();
		return Vector4f(seq.at(0).asFloat(), seq.at(1).asFloat(), seq.at(2).asFloat(), seq.at(3).asFloat());
	} else {
		throw Exception(getNodeDebugId() + " is not a vector4 type", HalleyExceptions::Resources);
	}
}

Range<float> ConfigNode::asFloatRange() const
{
	if (type == ConfigNodeType::Int2) {
		return Range<float>(vec2iData.x, vec2iData.y);
	} else if (type == ConfigNodeType::Float2) {
		return Range<float>(vec2fData.x, vec2fData.y);
	} else if (type == ConfigNodeType::Sequence) {
		const auto& seq = asSequence();
		return Range<float>(seq.at(0).asFloat(), seq.at(1).asFloat());
	} else {
		throw Exception(getNodeDebugId() + " is not a range type", HalleyExceptions::Resources);
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

void ConfigNode::ensureType(ConfigNodeType t)
{
	if (type != t) {
		switch (t) {
		case ConfigNodeType::Int:
			*this = 0;
			break;
		case ConfigNodeType::Int2:
			*this = Vector2i();
			break;
		case ConfigNodeType::Float:
			*this = 0.0f;
			break;
		case ConfigNodeType::Float2:
			*this = Vector2f();
			break;
		case ConfigNodeType::Sequence:
			*this = SequenceType();
			break;
		case ConfigNodeType::Map:
			*this = MapType();
			break;
		case ConfigNodeType::Bytes:
			*this = Bytes();
			break;
		case ConfigNodeType::String:
			*this = String();
			break;
		case ConfigNodeType::Undefined:
			reset();
			type = ConfigNodeType::Undefined;
			break;
		}
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

int ConfigNode::convertTo(Tag<int> tag) const
{
	return asInt();
}

float ConfigNode::convertTo(Tag<float> tag) const
{
	return asFloat();
}

bool ConfigNode::convertTo(Tag<bool> tag) const
{
	return asBool();
}

Vector2i ConfigNode::convertTo(Tag<Vector2i> tag) const
{
	return asVector2i();
}

Vector2f ConfigNode::convertTo(Tag<Vector2f> tag) const
{
	return asVector2f();
}

Vector4i ConfigNode::convertTo(Tag<Vector4i> tag) const
{
	return asVector4i();
}

Vector4f ConfigNode::convertTo(Tag<Vector4f> tag) const
{
	return asVector4f();
}

Range<float> ConfigNode::convertTo(Tag<Range<float>> tag) const
{
	return asFloatRange();
}

String ConfigNode::convertTo(Tag<String> tag) const
{
	return asString();
}

const Bytes& ConfigNode::convertTo(Tag<Bytes&> tag) const
{
	return asBytes();
}

ConfigNode ConfigNode::createDelta(const ConfigNode& from, const ConfigNode& to)
{
	if (from.getType() == to.getType()) {
		if (from.getType() == ConfigNodeType::Map) {
			return createMapDelta(from, to);
		}

		if (from.getType() == ConfigNodeType::Sequence) {
			return createSequenceDelta(from, to);
		}

		if (from == to) {
			// No change
			return ConfigNode(NoopType());
		}
	}

	// No delta coding available, return the outcome
	return ConfigNode(to);
}

ConfigNode ConfigNode::createMapDelta(const ConfigNode& from, const ConfigNode& to)
{
	// TODO
	return ConfigNode();
}

ConfigNode ConfigNode::createSequenceDelta(const ConfigNode& from, const ConfigNode& to)
{
	// TODO
	return ConfigNode();
}

void ConfigNode::applyDelta(const ConfigNode& delta)
{
	// TODO
}

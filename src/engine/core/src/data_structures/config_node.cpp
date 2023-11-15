#include "halley/data_structures/config_node.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/file_formats/config_file.h"
#include "halley/support/exception.h"
#include "../file_formats/config_file_serialization_state.h"
#include "halley/utils/hash.h"
using namespace Halley;

void EntityIdHolder::serialize(Serializer& s) const
{
	s << value;
}

void EntityIdHolder::deserialize(Deserializer& s)
{
	s >> value;
}

ConfigNode::ConfigNode()
{
}

ConfigNode::ConfigNode(const ConfigNode& other)
{
	operator=(other);
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

ConfigNode::ConfigNode(const std::string_view& value)
{
	operator=(value);
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

ConfigNode::ConfigNode(uint32_t value)
{
	operator=(value);
}

ConfigNode::ConfigNode(int64_t value)
{
	operator=(value);
}

ConfigNode::ConfigNode(EntityIdHolder value)
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

ConfigNode::ConfigNode(Vector3i value)
{
	operator=(value);
}

ConfigNode::ConfigNode(Vector3f value)
{
	operator=(value);
}

ConfigNode::ConfigNode(Vector4i value)
{
	operator=(value);
}

ConfigNode::ConfigNode(Vector4f value)
{
	operator=(value);
}

ConfigNode::ConfigNode(Rect4i value)
{
	operator=(value);
}

ConfigNode::ConfigNode(Rect4f value)
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

bool ConfigNode::removeKey(std::string_view key)
{
	auto& map = asMap();
	const auto iter = map.find(key);
	if (iter != map.end()) {
		map.erase(iter);
		return true;
	}
	return false;
}

ConfigNode::~ConfigNode()
{
	reset();
}

ConfigNode& ConfigNode::operator=(const ConfigNode& other)
{
	switch (other.type) {
		case ConfigNodeType::String:
			*this = other.asString();
			break;
		case ConfigNodeType::Sequence:
		case ConfigNodeType::DeltaSequence:
			*this = other.asSequence();
			break;
		case ConfigNodeType::Map:
		case ConfigNodeType::DeltaMap:
			*this = other.asMap();
			break;
		case ConfigNodeType::Int:
			*this = other.asInt();
			break;
		case ConfigNodeType::Int64:
			*this = other.asInt64();
			break;
		case ConfigNodeType::Bool:
			*this = other.asBool();
			break;
		case ConfigNodeType::EntityId:
			*this = other.asEntityId();
			break;
		case ConfigNodeType::Float:
			*this = other.asFloat();
			break;
		case ConfigNodeType::Int2:
		case ConfigNodeType::Idx:
			*this = other.asVector2i();
			break;
		case ConfigNodeType::Float2:
			*this = other.asVector2f();
			break;
		case ConfigNodeType::Bytes:
			*this = other.asBytes();
			break;
		case ConfigNodeType::Undefined:
		case ConfigNodeType::Noop:
		case ConfigNodeType::Del:
			break;
		default:
			throw Exception("Unknown configuration node type.", HalleyExceptions::Resources);
	}

	type = other.type;
	auxData = other.auxData;

	return *this;
}

ConfigNode& ConfigNode::operator=(ConfigNode&& other) noexcept
{
	reset();

	type = other.type;
	rawPtrData = other.rawPtrData;
	intData = other.intData;
	floatData = other.floatData;
	vec2iData = other.vec2iData;
	vec2fData = other.vec2fData;
#if defined(STORE_CONFIG_NODE_PARENTING)
	parent = std::move(other.parent);
#endif
	auxData = other.auxData;
	
	other.type = ConfigNodeType::Undefined;
	other.rawPtrData = nullptr;
	
	return *this;
}

ConfigNode& ConfigNode::operator=(bool value)
{
	reset();
	type = ConfigNodeType::Bool;
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

ConfigNode& ConfigNode::operator=(uint32_t value)
{
	reset();
	type = ConfigNodeType::Int;
	intData = value;
	return *this;
}

ConfigNode& ConfigNode::operator=(int64_t value)
{
	reset();
	type = ConfigNodeType::Int64;
	int64Data = value;
	return *this;
}

ConfigNode& ConfigNode::operator=(EntityIdHolder value)
{
	reset();
	type = ConfigNodeType::EntityId;
	int64Data = value.value;
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

ConfigNode& ConfigNode::operator=(Vector3i value)
{
	auto seq = SequenceType({ ConfigNode(value.x), ConfigNode(value.y), ConfigNode(value.z) });
	return *this = std::move(seq);
}

ConfigNode& ConfigNode::operator=(Vector3f value)
{
	auto seq = SequenceType({ ConfigNode(value.x), ConfigNode(value.y), ConfigNode(value.z) });
	return *this = std::move(seq);
}

ConfigNode& ConfigNode::operator=(Vector4i value)
{
	auto seq = SequenceType({ ConfigNode(value.x), ConfigNode(value.y), ConfigNode(value.z), ConfigNode(value.w) });
	return *this = std::move(seq);
}

ConfigNode& ConfigNode::operator=(Vector4f value)
{
	auto seq = SequenceType({ ConfigNode(value.x), ConfigNode(value.y), ConfigNode(value.z), ConfigNode(value.w) });
	return *this = std::move(seq);
}

ConfigNode& ConfigNode::operator=(Rect4i value)
{
	auto seq = SequenceType({ ConfigNode(value.getP1().x), ConfigNode(value.getP1().y), ConfigNode(value.getP2().x), ConfigNode(value.getP2().y) });
	return *this = std::move(seq);
}

ConfigNode& ConfigNode::operator=(Rect4f value)
{
	auto seq = SequenceType({ ConfigNode(value.getP1().x), ConfigNode(value.getP1().y), ConfigNode(value.getP2().x), ConfigNode(value.getP2().y) });
	return *this = std::move(seq);
}

ConfigNode& ConfigNode::operator=(Range<float> value)
{
	reset();
	type = ConfigNodeType::Float2;
	vec2fData = Vector2f(value.start, value.end);
	return *this;
}

ConfigNode& ConfigNode::operator=(Range<int> value)
{
	reset();
	type = ConfigNodeType::Int2;
	vec2iData = Vector2i(value.start, value.end);
	return *this;
}

ConfigNode& ConfigNode::operator=(const std::string_view& value)
{
	*this = String(value);
	return *this;
}

bool ConfigNode::operator==(const ConfigNode& other) const
{
	if (type != other.type) {
		return isEquivalent(other);
	}
	
	switch (type) {
		case ConfigNodeType::String:
			return asString() == other.asString();
		case ConfigNodeType::Sequence:
		case ConfigNodeType::DeltaSequence:
			return asSequence() == other.asSequence();
		case ConfigNodeType::Map:
		case ConfigNodeType::DeltaMap:
			return asMap() == other.asMap();
		case ConfigNodeType::Int:
			return asInt() == other.asInt();
		case ConfigNodeType::Bool:
			return asBool() == other.asBool();
		case ConfigNodeType::Int64:
			return asInt64() == other.asInt64();
		case ConfigNodeType::EntityId:
			return asEntityId().value == other.asEntityId().value;
		case ConfigNodeType::Float:
			return std::abs(asFloat() - other.asFloat()) < 0.00001f;
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
	bytesData = new Bytes(std::move(value));
	return *this;
}

ConfigNode& ConfigNode::operator=(gsl::span<const gsl::byte> bytes)
{
	reset();
	type = ConfigNodeType::Bytes;
	auto b = new Bytes(bytes.size_bytes());
	memcpy(b->data(), bytes.data(), bytes.size_bytes());
	bytesData = b;
	return *this;
}

ConfigNode& ConfigNode::operator=(MapType entry) 
{
	reset();
	type = ConfigNodeType::Map;
	mapData = new MapType(std::move(entry));
	return *this;
}

ConfigNode& ConfigNode::operator=(SequenceType entry) 
{
	reset();
	type = ConfigNodeType::Sequence;
	sequenceData = new SequenceType(std::move(entry));
	return *this;
}

ConfigNode& ConfigNode::operator=(const char* value)
{
	reset();
	type = ConfigNodeType::String;
	strData = new String(value);
	return *this;
}

ConfigNode& ConfigNode::operator=(String entry) 
{
	reset();
	type = ConfigNodeType::String;
	strData = new String(std::move(entry));
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
			s << asString();
			break;
		case ConfigNodeType::Sequence:
			s << asSequence();
			break;
		case ConfigNodeType::Map:
			s << asMap();
			break;
		case ConfigNodeType::Bool:
			s << asBool();
			break;
		case ConfigNodeType::Int:
			s << asInt();
			break;
		case ConfigNodeType::Int64:
			s << asInt64();
			break;
		case ConfigNodeType::EntityId:
			s << asEntityId().value;
			break;
		case ConfigNodeType::Float:
			s << asFloat();
			break;
		case ConfigNodeType::Int2:
		case ConfigNodeType::Idx:
			s << asVector2i();
			break;
		case ConfigNodeType::Float2:
			s << asVector2f();
			break;
		case ConfigNodeType::Bytes:
			s << asBytes();
			break;
		case ConfigNodeType::DeltaMap:
			s << asMap();
			s << auxData;
			break;
		case ConfigNodeType::DeltaSequence:
			s << asSequence();
			s << auxData;
			break;
		case ConfigNodeType::Undefined:
		case ConfigNodeType::Del:
		case ConfigNodeType::Noop:
		{
			break;
		}
		default:
			throw Exception("Unknown configuration node type.", HalleyExceptions::Resources);
	}

	const auto* state = s.getState<ConfigFileSerializationState>();
	if (state && state->storeFilePosition) {
#if defined(STORE_CONFIG_NODE_PARENTING)
		if (parent) {
			s << parent->line;
			s << parent->column;
		} else {
			s << 0 << 0;
		}
#else
		s << 0 << 0;
#endif
	}
}

void ConfigNode::deserialize(Deserializer& s)
{
	ConfigNodeType incomingType;
	s >> incomingType;

	switch (incomingType) {
		case ConfigNodeType::String:
			deserializeContents<String>(s);
			break;
		case ConfigNodeType::Sequence:
			deserializeContents<SequenceType>(s);
			break;
		case ConfigNodeType::Map:
			deserializeContents<MapType>(s);
			break;
		case ConfigNodeType::Bool:
			deserializeContents<bool>(s);
			break;
		case ConfigNodeType::Int:
			deserializeContents<int>(s);
			break;
		case ConfigNodeType::Int64:
			deserializeContents<int64_t>(s);
			break;
		case ConfigNodeType::EntityId:
			deserializeContents<EntityIdHolder>(s);
			break;
		case ConfigNodeType::Float:
			deserializeContents<float>(s);
			break;
		case ConfigNodeType::Int2:
		case ConfigNodeType::Idx:
			deserializeContents<Vector2i>(s);
			break;
		case ConfigNodeType::Float2:
			deserializeContents<Vector2f>(s);
			break;
		case ConfigNodeType::Bytes:
			deserializeContents<Bytes>(s);
			break;
		case ConfigNodeType::DeltaMap:
			deserializeContents<MapType>(s);
			s >> auxData;
			break;
		case ConfigNodeType::DeltaSequence:
			deserializeContents<SequenceType>(s);
			s >> auxData;
			break;
		case ConfigNodeType::Noop:
		case ConfigNodeType::Del:
		case ConfigNodeType::Undefined:
		{
			break;
		}
		default:
			throw Exception("Unknown configuration node type: " + toString(int(incomingType)), HalleyExceptions::Resources);
	}

	type = incomingType;

	const auto state = s.getState<ConfigFileSerializationState>();
	if (state && state->storeFilePosition) {
		[[maybe_unused]] int line;
		[[maybe_unused]] int column;
		s >> line >> column;

#if defined(STORE_CONFIG_NODE_PARENTING)
		if (parent) {
			parent->line = line;
			parent->column = column;
		}
#endif
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
	} else if (type == ConfigNodeType::Int64) {
		return int(int64Data);
	} else if (type == ConfigNodeType::EntityId && int64Data == -1) {
		return -1;
	} else if (type == ConfigNodeType::Bool) {
		return intData;
	} else {
		throw Exception(getNodeDebugId() + " cannot be converted to int.", HalleyExceptions::Resources);
	}
}

int64_t ConfigNode::asInt64() const
{
	if (type == ConfigNodeType::Int64 || type == ConfigNodeType::EntityId) {
		return int64Data;
	} else if (type == ConfigNodeType::Int) {
		return intData;
	} else if (type == ConfigNodeType::Float) {
		return int(floatData);
	} else if (type == ConfigNodeType::Bool) {
		return intData;
	} else if (type == ConfigNodeType::String) {
		return asString().toInteger();
	} else {
		throw Exception(getNodeDebugId() + " cannot be converted to int.", HalleyExceptions::Resources);
	}
}

EntityIdHolder ConfigNode::asEntityId() const
{
	if (type == ConfigNodeType::EntityId || type == ConfigNodeType::Int64) {
		return EntityIdHolder{ int64Data };
	} else if (type == ConfigNodeType::Int && intData == -1) {
		return EntityIdHolder{};
	} else if (type == ConfigNodeType::Float && std::abs(floatData + 1.0f) < 0.0001f) {
		return EntityIdHolder{};
	} else {
		throw Exception(getNodeDebugId() + " cannot be converted to EntityId.", HalleyExceptions::Resources);
	}
}

float ConfigNode::asFloat() const
{
	if (type == ConfigNodeType::Int) {
		return float(intData);
	} else if (type == ConfigNodeType::Float) {
		return floatData;
	} else if (type == ConfigNodeType::Int64) {
		return float(int64Data);
	} else if (type == ConfigNodeType::Bool) {
		return float(intData);
	} else if (type == ConfigNodeType::String) {
		return asString().toFloat();
	} else if (type == ConfigNodeType::EntityId && int64Data == -1) {
		return -1.0f;
	} else {
		throw Exception(getNodeDebugId() + " cannot be converted to float.", HalleyExceptions::Resources);
	}
}

bool ConfigNode::asBool() const
{
	if (type == ConfigNodeType::Int || type == ConfigNodeType::Bool) {
		return intData != 0;
	} else if (type == ConfigNodeType::Float) {
		return floatData != 0;
	} else if (type == ConfigNodeType::Int64) {
		return int64Data != 0;
	} else if (type == ConfigNodeType::String) {
		const auto& str = asStringView();
		if (str == "true") {
			return true;
		} else if (str == "false" || str == "0") {
			return false;
		}
		return !str.empty();
	} else if (type == ConfigNodeType::EntityId) {
		return int64Data != -1;
	}
	return type != ConfigNodeType::Undefined;
}

Vector2i ConfigNode::asVector2i() const
{
	if (type == ConfigNodeType::Int2 || type == ConfigNodeType::Idx) {
		return vec2iData;
	} else if (type == ConfigNodeType::Float2) {
		return Vector2i(vec2fData);
	} else if (type == ConfigNodeType::Int || type == ConfigNodeType::Float) {
		return Vector2i(asInt(), 0);
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
	} else if (type == ConfigNodeType::Int || type == ConfigNodeType::Float) {
		return Vector2f(asFloat(), 0);
	} else if (type == ConfigNodeType::Sequence) {
		auto& seq = asSequence();
		return Vector2f(seq.at(0).asFloat(), seq.at(1).asFloat());
	} else {
		throw Exception(getNodeDebugId() + " is not a vector2 type", HalleyExceptions::Resources);
	}
}

Vector3i ConfigNode::asVector3i() const
{
	if (type == ConfigNodeType::Sequence) {
		auto& seq = asSequence();
		return Vector3i(seq.at(0).asInt(), seq.size() >= 2 ? seq.at(1).asInt() : 0, seq.size() >= 3 ? seq.at(2).asInt() : 0);
	} else if (type == ConfigNodeType::Float2 || type == ConfigNodeType::Int2) {
		return Vector3i(asVector2i(), 0);
	} else {
		throw Exception(getNodeDebugId() + " is not a vector3 type", HalleyExceptions::Resources);
	}
}

Vector3f ConfigNode::asVector3f() const
{
	if (type == ConfigNodeType::Sequence) {
		auto& seq = asSequence();
		return Vector3f(seq.at(0).asFloat(), seq.size() >= 2 ? seq.at(1).asFloat() : 0.0f, seq.size() >= 3 ? seq.at(2).asFloat() : 0.0f);
	} else if (type == ConfigNodeType::Float2 || type == ConfigNodeType::Int2) {
		return Vector3f(asVector2f(), 0);
	} else {
		throw Exception(getNodeDebugId() + " is not a vector3 type", HalleyExceptions::Resources);
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

Rect4i ConfigNode::asRect4i() const
{
	if (type == ConfigNodeType::Sequence) {
		auto& seq = asSequence();
		return Rect4i(Vector2i(seq.at(0).asInt(), seq.at(1).asInt()), Vector2i(seq.at(2).asInt(), seq.at(3).asInt()));
	} else {
		throw Exception(getNodeDebugId() + " is not a rect4 type", HalleyExceptions::Resources);
	}
}

Rect4f ConfigNode::asRect4f() const
{
	if (type == ConfigNodeType::Sequence) {
		auto& seq = asSequence();
		return Rect4f(Vector2f(seq.at(0).asFloat(), seq.at(1).asFloat()), Vector2f(seq.at(2).asFloat(), seq.at(3).asFloat()));
	} else {
		throw Exception(getNodeDebugId() + " is not a rect4 type", HalleyExceptions::Resources);
	}
}

Range<float> ConfigNode::asFloatRange() const
{
	if (type == ConfigNodeType::Int2) {
		return Range<float>(static_cast<float>(vec2iData.x), static_cast<float>(vec2iData.y));
	} else if (type == ConfigNodeType::Float2) {
		return Range<float>(vec2fData.x, vec2fData.y);
	} else if (type == ConfigNodeType::Sequence) {
		const auto& seq = asSequence();
		return Range<float>(seq.at(0).asFloat(), seq.at(1).asFloat());
	} else if (type == ConfigNodeType::Int || type == ConfigNodeType::Float) {
		const auto v = asFloat();
		return Range<float>(v, v);
	} else {
		throw Exception(getNodeDebugId() + " is not a range type", HalleyExceptions::Resources);
	}
}

const Bytes& ConfigNode::asBytes() const
{
	if (type == ConfigNodeType::Bytes) {
		return *bytesData;
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

Vector3i ConfigNode::asVector3i(Vector3i defaultValue) const
{
	if (type == ConfigNodeType::Undefined) {
		return defaultValue;
	} else {
		return asVector3i();
	}
}

Vector3f ConfigNode::asVector3f(Vector3f defaultValue) const
{
	if (type == ConfigNodeType::Undefined) {
		return defaultValue;
	} else {
		return asVector3f();
	}
}

Vector4i ConfigNode::asVector4i(Vector4i defaultValue) const
{
	if (type == ConfigNodeType::Undefined) {
		return defaultValue;
	} else {
		return asVector4i();
	}
}

Vector4f ConfigNode::asVector4f(Vector4f defaultValue) const
{
	if (type == ConfigNodeType::Undefined) {
		return defaultValue;
	} else {
		return asVector4f();
	}
}

Rect4i ConfigNode::asRect4i(Rect4i defaultValue) const
{
	if (type == ConfigNodeType::Undefined) {
		return defaultValue;
	} else {
		return asRect4i();
	}
}

Rect4f ConfigNode::asRect4f(Rect4f defaultValue) const
{
	if (type == ConfigNodeType::Undefined) {
		return defaultValue;
	} else {
		return asRect4f();
	}
}

Range<float> ConfigNode::asFloatRange(Range<float> defaultValue) const
{
	if (type == ConfigNodeType::Undefined) {
		return defaultValue;
	} else {
		return asFloatRange();
	}
}

String ConfigNode::asString() const
{
	if (type == ConfigNodeType::String) {
		return *strData;
	} else if (type == ConfigNodeType::Int) {
		return toString(asInt());
	} else if (type == ConfigNodeType::Int64) {
		return toString(asInt64());
	} else if (type == ConfigNodeType::Bool) {
		return asBool() ? "true" : "false";
	} else if (type == ConfigNodeType::EntityId) {
		return toString(asEntityId().value);
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
		String result = "{";
		bool first = true;
		for (auto& [k, v] : asMap()) {
			if (!first) {
				result += ", ";
			}
			first = false;
			result += k + ": " + v.asString();
		}
		result += "}";
		return result;
	} else if (type == ConfigNodeType::Undefined) {
		return "null";
	} else {
		throw Exception("Can't convert " + getNodeDebugId() + " from " + toString(getType()) + " to String.", HalleyExceptions::Resources);
	}
}

std::string_view ConfigNode::asStringView() const
{
	if (type == ConfigNodeType::String) {
		return *strData;
	} else {
		throw Exception("Can't convert " + getNodeDebugId() + " from " + toString(getType()) + " to StringView.", HalleyExceptions::Resources);
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

int64_t ConfigNode::asInt64(int64_t defaultValue) const
{
	if (type == ConfigNodeType::Undefined) {
		return defaultValue;
	} else {
		return asInt64();
	}
}

EntityIdHolder ConfigNode::asEntityId(EntityIdHolder defaultValue) const
{
	if (type == ConfigNodeType::Undefined) {
		return defaultValue;
	} else {
		return asEntityId();
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

String ConfigNode::asString(const std::string_view& defaultValue) const
{
	if (type == ConfigNodeType::Undefined) {
		return defaultValue;
	} else {
		return asString();
	}
}

std::string_view ConfigNode::asStringView(const std::string_view& defaultValue) const
{
	if (type == ConfigNodeType::Undefined) {
		return defaultValue;
	} else {
		return asStringView();
	}
}

const ConfigNode::SequenceType& ConfigNode::asSequence() const
{
	if (type == ConfigNodeType::Sequence || type == ConfigNodeType::DeltaSequence) {
		return *sequenceData;
	} else {
		throw Exception(getNodeDebugId() + " is not a sequence type", HalleyExceptions::Resources);
	}
}

const ConfigNode::MapType& ConfigNode::asMap() const
{
	if (type == ConfigNodeType::Map || type == ConfigNodeType::DeltaMap) {
		return *mapData;
	} else {
		throw Exception(getNodeDebugId() + " is not a map type", HalleyExceptions::Resources);
	}
}

ConfigNode::SequenceType& ConfigNode::asSequence()
{
	if (type == ConfigNodeType::Sequence || type == ConfigNodeType::DeltaSequence) {
		return *sequenceData;
	} else {
		throw Exception(getNodeDebugId() + " is not a sequence type", HalleyExceptions::Resources);
	}
}

ConfigNode::MapType& ConfigNode::asMap()
{
	if (type == ConfigNodeType::Map || type == ConfigNodeType::DeltaMap) {
		return *mapData;
	} else {
		throw Exception(getNodeDebugId() + " is not a map type", HalleyExceptions::Resources);
	}
}

size_t ConfigNode::getSequenceSize(size_t defaultValue) const
{
	if (type == ConfigNodeType::Sequence || type == ConfigNodeType::DeltaSequence) {
		return asSequence().size();
	} else {
		return defaultValue;
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
		case ConfigNodeType::Int64:
			*this = int64_t(0);
			break;
		case ConfigNodeType::Bool:
			*this = false;
			break;
		case ConfigNodeType::EntityId:
			*this = EntityIdHolder{};
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

bool ConfigNode::hasKey(std::string_view key) const
{
	if (type == ConfigNodeType::Map || type == ConfigNodeType::DeltaMap) {
		auto& map = asMap();
		auto iter = map.find(key);
		return iter != map.end() && iter->second.getType() != ConfigNodeType::Undefined;
	} else {
		return false;
	}
}

ConfigNode& ConfigNode::operator[](std::string_view key)
{
	return asMap()[key];
}

ConfigNode& ConfigNode::operator[](size_t idx)
{
	return asSequence().at(idx);
}

const ConfigNode& ConfigNode::operator[](std::string_view key) const
{
	const auto& map = asMap();
	const auto iter = map.find(key);
	if (iter != map.end()) {
		return iter->second;
	} else {
#if defined(STORE_CONFIG_NODE_PARENTING)
		undefinedConfigNode.setParent(this, -1);
		undefinedConfigNode.parent->file = parent ? parent->file : nullptr;
#endif
		//undefinedConfigNodeName = key;
		return undefinedConfigNode;
	}
}

const ConfigNode& ConfigNode::operator[](size_t idx) const
{
	return asSequence().at(idx);
}

ConfigNode& ConfigNode::at(std::string_view key)
{
	auto& map = asMap();
	const auto iter = map.find(key);
	if (iter != map.end()) {
		return iter->second;
	} else {
#if defined(STORE_CONFIG_NODE_PARENTING)
		undefinedConfigNode.setParent(this, -1);
		undefinedConfigNode.parent->file = parent ? parent->file : nullptr;
#endif
		//undefinedConfigNodeName = key;
		return undefinedConfigNode;
	}
}

const ConfigNode& ConfigNode::at(std::string_view key) const
{
	const auto& map = asMap();
	const auto iter = map.find(key);
	if (iter != map.end()) {
		return iter->second;
	} else {
#if defined(STORE_CONFIG_NODE_PARENTING)
		undefinedConfigNode.setParent(this, -1);
		undefinedConfigNode.parent->file = parent ? parent->file : nullptr;
#endif
		//undefinedConfigNodeName = key;
		return undefinedConfigNode;
	}
}

Vector<ConfigNode>::iterator ConfigNode::begin()
{
	return asSequence().begin();
}

Vector<ConfigNode>::iterator ConfigNode::end()
{
	return asSequence().end();
}

Vector<ConfigNode>::const_iterator ConfigNode::begin() const
{
	return asSequence().begin();
}

Vector<ConfigNode>::const_iterator ConfigNode::end() const
{
	return asSequence().end();
}

void ConfigNode::reset()
{
	if (type == ConfigNodeType::Map || type == ConfigNodeType::DeltaMap) {
		delete mapData;
	} else if (type == ConfigNodeType::Sequence || type == ConfigNodeType::DeltaSequence) {
		delete sequenceData;
	} else if (type == ConfigNodeType::Bytes) {
		delete bytesData;
	} else if (type == ConfigNodeType::String) {
		delete strData;
	}
	rawPtrData = nullptr;
	type = ConfigNodeType::Undefined;
}

void ConfigNode::setOriginalPosition(int l, int c)
{
#if defined(STORE_CONFIG_NODE_PARENTING)
	if (!parent) {
		parent = std::make_unique<ParentingInfo>();
	}
	parent->line = l;
	parent->column = c;
#endif
}

void ConfigNode::setParent(const ConfigNode* p, int idx)
{
#if defined(STORE_CONFIG_NODE_PARENTING)
	if (!parent) {
		parent = std::make_unique<ParentingInfo>();
	}
	parent->node = p;
	parent->idx = idx;
#endif
}

void ConfigNode::propagateParentingInformation(const ConfigFile* file)
{
#if defined(STORE_CONFIG_NODE_PARENTING)
	if (!parent) {
		parent = std::make_unique<ParentingInfo>();
	}
	parent->file = file;
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
#endif
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
		case ConfigNodeType::Bool:
			value = asBool() ? "true" : "false";
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

#if defined(STORE_CONFIG_NODE_PARENTING)
	if (parent) {
		String assetId = "unknown";
		if (parent->file) {
			assetId = parent->file->getAssetId();
		}
		return "Node \"" + backTrackFullNodeName() + "\" (" + value + ") at \"" + assetId + "(" + toString(parent->line + 1) + ":" + toString(parent->column + 1) + ")\"";
	}
#endif
	
	return "Node (" + value + ")";
}

String ConfigNode::backTrackFullNodeName() const
{
#if defined(STORE_CONFIG_NODE_PARENTING)
	if (parent && parent->node) {
		if (parent->node->type == ConfigNodeType::Sequence) {
			return parent->node->backTrackFullNodeName() + "[" + toString(parent->idx) + "]";
		} else if (parent->node->type == ConfigNodeType::Map) {
			auto& parentMap = parent->node->asMap();
			int i = 0;
			String name = "?";
			if (parent->idx == -1) {
				name = ConfigNode::undefinedConfigNodeName;
			} else {
				for (auto& e: parentMap) {
					if (i++ == parent->idx) {
						name = e.first;
						break;
					}
				}
			}
			return parent->node->backTrackFullNodeName() + "." + name;
		} else {
			return "?";
		}
	} else {
		return "~";
	}
#else
	return "";
#endif
}

int ConfigNode::convertTo(Tag<int> tag) const
{
	return asInt();
}

int64_t ConfigNode::convertTo(Tag<int64_t> tag) const
{
	return asInt64();
}

EntityIdHolder ConfigNode::convertTo(Tag<EntityIdHolder> tag) const
{
	return asEntityId();
}

float ConfigNode::convertTo(Tag<float> tag) const
{
	return asFloat();
}

bool ConfigNode::convertTo(Tag<bool> tag) const
{
	return asBool();
}

uint8_t ConfigNode::convertTo(Tag<uint8_t> tag) const
{
	return asInt();
}

uint16_t ConfigNode::convertTo(Tag<uint16_t> tag) const
{
	return asInt();
}

uint32_t ConfigNode::convertTo(Tag<uint32_t> tag) const
{
	return asInt();
}

uint64_t ConfigNode::convertTo(Tag<uint64_t> tag) const
{
	return asInt64();
}

Vector2i ConfigNode::convertTo(Tag<Vector2i> tag) const
{
	return asVector2i();
}

Vector2f ConfigNode::convertTo(Tag<Vector2f> tag) const
{
	return asVector2f();
}

Vector3i ConfigNode::convertTo(Tag<Vector3i> tag) const
{
	return asVector3i();
}

Vector3f ConfigNode::convertTo(Tag<Vector3f> tag) const
{
	return asVector3f();
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

bool ConfigNode::isNullOrEmpty() const
{
	switch (type) {
	case ConfigNodeType::Undefined:
		return true;
	case ConfigNodeType::Map:
	case ConfigNodeType::DeltaMap:
		return asMap().empty();
	case ConfigNodeType::Sequence:
	case ConfigNodeType::DeltaSequence:
		return asSequence().empty();
	default:
		return false;
	}
}

bool ConfigNode::BreadCrumb::hasKeyAt(const String& key, int depth) const
{
	if (depth == 0) {
		return this->key == key;
	} else if (prev) {
		return prev->hasKeyAt(key, depth - 1);
	} else {
		return false;
	}
}

bool ConfigNode::BreadCrumb::hasIndexAt(int idx, int depth) const
{
	if (depth == 0) {
		return this->idx == idx;
	} else if (prev) {
		return prev->hasIndexAt(idx, depth - 1);
	} else {
		return false;
	}	
}

ConfigNode ConfigNode::createDelta(const ConfigNode& from, const ConfigNode& to, const IDeltaCodeHints* hints)
{
	return doCreateDelta(from, to, BreadCrumb(), hints);
}

ConfigNode ConfigNode::applyDelta(const ConfigNode& from, const ConfigNode& delta)
{
	auto result = ConfigNode(from);
	result.applyDelta(delta);
	return result;
}

ConfigNode ConfigNode::doCreateDelta(const ConfigNode& from, const ConfigNode& to, const BreadCrumb& breadCrumb, const IDeltaCodeHints* hints)
{
	// This allows things (e.g. ConfigNodeSerializer) to return Noop for fields that should skip the delta, for example when a field is not relevant to save data
	if (to.getType() == ConfigNodeType::Noop) {
		return ConfigNode(NoopType());
	}
	
	if (from.getType() == to.getType()) {
		if (hints && hints->shouldBypass(breadCrumb)) {
			return ConfigNode(NoopType());
		}

		if (from.getType() == ConfigNodeType::Map) {
			auto delta = createMapDelta(from, to, breadCrumb, hints);
			delta.auxData = breadCrumb.idx.value_or(0);
			return delta;
		}

		if (from.getType() == ConfigNodeType::Sequence) {
			auto delta = createSequenceDelta(from, to, breadCrumb, hints);
			delta.auxData = breadCrumb.idx.value_or(0);
			return delta;
		}
	}

	// Can compare equals even if the types are different (e.g. float 12.0 and int 12)
	if (from == to) {
		// No change
		return ConfigNode(NoopType());
	}

	// If one is undefined, consider no change if the other is a sequence/map that is empty
	if ((from.getType() == ConfigNodeType::Undefined && to.isNullOrEmpty()) || (to.getType() == ConfigNodeType::Undefined && from.isNullOrEmpty())) {
		if (hints && hints->areNullAndEmptyEquivalent(breadCrumb)) {
			return ConfigNode(NoopType());
		}
	}

	// No delta coding available, return the outcome
	return ConfigNode(to);
}

ConfigNode ConfigNode::createMapDelta(const ConfigNode& from, const ConfigNode& to, const BreadCrumb& breadCrumb, const IDeltaCodeHints* hints)
{
	const auto& fromMap = from.asMap();
	const auto& toMap = to.asMap();

	// Shortcut if from is empty
	if (fromMap.empty()) {
		if (toMap.empty()) {
			return ConfigNode(NoopType());
		}
		auto result = ConfigNode(toMap);
		result.type = ConfigNodeType::DeltaMap;
		return result;
	}
	
	auto result = ConfigNode(MapType());
	result.type = ConfigNodeType::DeltaMap;
			
	// Store the new keys if there's a change
	for (const auto& [k, v]: toMap) {
		if (v.getType() != ConfigNodeType::Noop) {
			const auto origIter = fromMap.find(k);

			if (origIter != fromMap.end()) {
				// Was present before, delta compress and store if it's different
				auto delta = doCreateDelta(origIter->second, v, BreadCrumb(breadCrumb, k), hints);
				if (delta.getType() != ConfigNodeType::Noop) {
					result[k] = std::move(delta);
				}
			} else {
				// Is new, store all of it
				result[k] = ConfigNode(v);
			}			
		}
	}

	// Remove old keys if they're deleted
	if (!hints || hints->canDeleteAnyKey()) {
		for (const auto& [k, v]: fromMap) {
			const auto newIter = toMap.find(k);
			if (newIter == toMap.end()) {
				// Key deleted
				if (!hints || hints->canDeleteKey(k, breadCrumb)) {
					result[k] = ConfigNode(DelType());
				}
			}
		}
	}

	// If it doesn't change the original map, just return noop
	if (result.asMap().empty()) {
		return ConfigNode(NoopType());
	}

	// TODO: if it replaces ALL of the original map, just return the new map?
	
	return result;
}

ConfigNode ConfigNode::createSequenceDelta(const ConfigNode& from, const ConfigNode& to, const BreadCrumb& breadCrumb, const IDeltaCodeHints* hints)
{
	auto tryExtend = [] (Vector<ConfigNode>& seq, int idx) -> bool
	{
		if (seq.empty() || seq.back().getType() != ConfigNodeType::Idx) {
			return false;
		}
		const auto prev = seq.back().asVector2i();
		if (prev.x + prev.y == idx) {
			seq.back() = IdxType(prev.x, idx - prev.x + 1);
			return true;
		}
		return false;
	};
	
	auto result = ConfigNode(SequenceType());
	result.type = ConfigNodeType::DeltaSequence;
	
	const auto& fromSeq = from.asSequence();
	const auto& toSeq = to.asSequence();
	auto& resultSeq = result.asSequence();

	if (fromSeq == toSeq) {
		return ConfigNode(NoopType());
	}
	if (!hints) {
		return ConfigNode(to);
	}

	bool hasNewData = false;
	size_t refCount = 0;

	for (size_t curIdx = 0; curIdx != toSeq.size(); ++curIdx) {
		const auto& curVal = toSeq[curIdx];
		
		// Ask the hints for the best element to compare with
		std::optional<size_t> matchIdx;
		if (hints) {
			matchIdx = hints->getSequenceMatch(fromSeq, curVal, curIdx, breadCrumb);
		}

		if (matchIdx) {
			// Found a match, delta code to that
			auto delta = result.doCreateDelta(fromSeq.at(matchIdx.value()), curVal, BreadCrumb(breadCrumb, static_cast<int>(curIdx)), hints);
			if (delta.getType() == ConfigNodeType::Noop) {
				// Nothing to encode, store a reference
				const int startIdx = static_cast<int>(matchIdx.value());
				++refCount;

				// Check if the last value can simply be extended
				if (!tryExtend(resultSeq, startIdx)) {
					resultSeq.emplace_back(ConfigNode(IdxType(startIdx, 1)));
				}
			} else {
				// Store delta
				resultSeq.emplace_back(std::move(delta));
				hasNewData = true;
			}
		} else {
			// Just store original
			resultSeq.push_back(curVal);
			hasNewData = true;
		}
	}

	// Check if it's just a permutation of the old data
	if (!hasNewData && refCount == fromSeq.size()) {
		if (hints && !hints->doesSequenceOrderMatter(breadCrumb)) {
			Vector<char> matches(fromSeq.size(), 0);
			for (const auto& e: resultSeq) {
				const auto range = e.asVector2i();
				for (int i = range.x; i < range.x + range.y; ++i) {
					++matches[i];
				}
			}
			bool redundant = true;
			for (auto& e: matches) {
				if (e != 1) {
					redundant = false;
					break;
				}
			}
			if (redundant) {
				return ConfigNode(NoopType());
			}
		}
	}

	if (resultSeq.size() == 1 && resultSeq.back().getType() == ConfigNodeType::Idx) {
		const auto idxData = resultSeq.back().asVector2i();
		if (idxData.x == 0 && idxData.y == static_cast<int>(fromSeq.size())) {
			return ConfigNode(NoopType());
		}
	}

	return result;
}

void ConfigNode::applyDelta(const ConfigNode& delta)
{
	if (delta.getType() == ConfigNodeType::Noop) {
		// Nothing to do
		return;
	}
	
	if (getType() == ConfigNodeType::Map && delta.getType() == ConfigNodeType::DeltaMap) {
		applyMapDelta(delta);
	} else if (getType() == ConfigNodeType::Sequence && delta.getType() == ConfigNodeType::DeltaSequence) {
		applySequenceDelta(delta);
	} else if (delta.getType() == ConfigNodeType::Idx || delta.getType() == ConfigNodeType::Del) {
		throw Exception("Invalid ConfigNode delta type at this position", HalleyExceptions::Utils);
	} else {
		*this = ConfigNode(delta);
	}
}

void ConfigNode::decayDeltaArtifacts()
{
	switch (type) {
	case ConfigNodeType::Map:
	case ConfigNodeType::DeltaMap:
		for (auto& [k, v]: asMap()) {
			v.decayDeltaArtifacts();
		}
		type = ConfigNodeType::Map;
		break;
	case ConfigNodeType::Sequence:
	case ConfigNodeType::DeltaSequence:
		for (auto& v: asSequence()) {
			v.decayDeltaArtifacts();
		}
		type = ConfigNodeType::Sequence;
		break;
	case ConfigNodeType::Idx:
	case ConfigNodeType::Noop:
	case ConfigNodeType::Del:
		reset();
		break;
	}
}

ConfigNodeType ConfigNode::getPromotedType(gsl::span<const ConfigNodeType> types, bool promoteUndefined)
{
	ConfigNodeType result = ConfigNodeType::Undefined;
	
	if (!types.empty()) {
		result = types[0];
		for (size_t i = 1; i < types.size(); ++i) {
			const auto a = types[i - 1];
			const auto b = types[i];

			if (a != b) {
				if (isScalarType(a, promoteUndefined) && isScalarType(b, promoteUndefined)) {
					result = (a == ConfigNodeType::Float || b == ConfigNodeType::Float) ? ConfigNodeType::Float : ((a == ConfigNodeType::Int64 || b == ConfigNodeType::Int64) ? ConfigNodeType::Int64 : ConfigNodeType::Int);
				} else {
					const bool aIsVec2 = isVector2Type(a, promoteUndefined);
					const bool bIsVec2 = isVector2Type(b, promoteUndefined);

					if (aIsVec2 && bIsVec2) {
						result = (a == ConfigNodeType::Float2 || b == ConfigNodeType::Float2) ? ConfigNodeType::Float2 : ConfigNodeType::Int2;
					} else if ((aIsVec2 || bIsVec2) && (a == ConfigNodeType::Sequence || b == ConfigNodeType::Sequence)) {
						return ConfigNodeType::Float2;
					} else {
						return ConfigNodeType::Undefined;
					}
				}
			}
		}
	}

	return result;
}

bool ConfigNode::isScalarType(ConfigNodeType type, bool acceptUndefined)
{
	return type == ConfigNodeType::Int || type == ConfigNodeType::Float || type == ConfigNodeType::Int64 || type == ConfigNodeType::Bool || (acceptUndefined && type == ConfigNodeType::Undefined);
}

bool ConfigNode::isVector2Type(ConfigNodeType type, bool acceptUndefined)
{
	return type == ConfigNodeType::Int2 || type == ConfigNodeType::Float2 || (acceptUndefined && type == ConfigNodeType::Undefined);
}

void ConfigNode::feedToHash(Hash::Hasher& hasher) const
{
	hasher.feed(type);
	switch (type) {
	case ConfigNodeType::Int:
	case ConfigNodeType::Bool:
		hasher.feed(intData);
		break;
	case ConfigNodeType::Int2:
		hasher.feed(vec2iData);
		break;
	case ConfigNodeType::Int64:
	case ConfigNodeType::EntityId:
		hasher.feed(int64Data);
		break;
	case ConfigNodeType::Float:
		hasher.feed(floatData);
		break;
	case ConfigNodeType::Float2:
		hasher.feed(vec2fData);
		break;
	case ConfigNodeType::Sequence:
		hasher.feed(asSequence().size());
		for (const auto& e: asSequence()) {
			e.feedToHash(hasher);
		}
		break;
	case ConfigNodeType::Map:
		for (const auto& [k, v]: asMap()) {
			if (v.getType() != ConfigNodeType::Undefined) {
				hasher.feed(k);
				v.feedToHash(hasher);
			}
		}
		break;
	case ConfigNodeType::Bytes:
		hasher.feedBytes(bytesData->byte_span());
		break;
	case ConfigNodeType::String:
		hasher.feed(*strData);
		break;
	case ConfigNodeType::Undefined:
		break;
	}
}

void ConfigNode::applyMapDelta(const ConfigNode& delta)
{
	auto& myMap = asMap();
	const auto& deltaMap = delta.asMap();

	for (const auto& [k, v]: deltaMap) {
		if (v.getType() == ConfigNodeType::Del) {
			// Erase existing
			myMap.erase(k);
		} else {
			const auto iter = myMap.find(k);
			if (iter != myMap.end()) {
				// Update existing
				iter->second.applyDelta(v);
			} else if (v.getType() != ConfigNodeType::Noop) {
				// Insert new
				myMap[k] = ConfigNode(v);
			}
		}
	}
}

void ConfigNode::applySequenceDelta(const ConfigNode& delta)
{
	const auto& mySeq = asSequence();
	const auto& deltaSeq = delta.asSequence();
	SequenceType result;

	for (const auto& v: deltaSeq) {
		if (v.getType() == ConfigNodeType::Idx) {
			// Simple index copying
			const auto indices = v.asVector2i();
			for (int i = indices.x; i < indices.x + indices.y; ++i) {
				result.emplace_back(mySeq.at(i));
			}
		} else if (v.getType() == ConfigNodeType::DeltaMap || v.getType() == ConfigNodeType::DeltaSequence) {
			// Apply delta
			auto value = ConfigNode(mySeq.at(v.auxData));
			value.applyDelta(v);
			result.emplace_back(std::move(value));
		} else {
			// New value
			Ensures(v.getType() != ConfigNodeType::Noop);
			result.emplace_back(v);
		}
	}

	*this = std::move(result);
}

bool ConfigNode::isEquivalent(const ConfigNode& other) const
{
	if (type < other.type) {
		return isEquivalentStrictOrder(other);
	} else {
		return other.isEquivalentStrictOrder(*this);
	}
}

bool ConfigNode::isEquivalentStrictOrder(const ConfigNode& other) const
{
	Expects(type < other.type);

	if (type == ConfigNodeType::Int && other.type == ConfigNodeType::Float) {
		return std::abs(asFloat() - other.asFloat()) < 0.00001f;
	}
	if (type == ConfigNodeType::Int && other.type == ConfigNodeType::Bool) {
		return asInt() == other.asInt();
	}
	if (type == ConfigNodeType::Sequence && other.type == ConfigNodeType::Int2 && asSequence().size() >= 2) {
		return asVector2i() == other.asVector2i();
	}
	if (type == ConfigNodeType::Sequence && other.type == ConfigNodeType::Float2 && asSequence().size() >= 2) {
		return asVector2f() == other.asVector2f();
	}
	
	return false;
}

size_t ConfigNode::getSizeBytes() const
{
	size_t result = sizeof(ConfigNode);

	switch (type) {
	case ConfigNodeType::Bytes:
		result += sizeof(Bytes) + bytesData->size();
		break;
	case ConfigNodeType::Map:
	case ConfigNodeType::DeltaMap:
		result += sizeof(MapType);
		for (auto& e: *mapData) {
			result += e.first.getSizeBytes();
			result += e.second.getSizeBytes();
		}
		break;
	case ConfigNodeType::DeltaSequence:
	case ConfigNodeType::Sequence:
		result += sizeof(SequenceType);
		for (auto& e: *sequenceData) {
			result += e.getSizeBytes();
		}
		break;
	case ConfigNodeType::String:
		result += sizeof(String);
		result += strData->getSizeBytes();
		break;
	}

	return result;
}

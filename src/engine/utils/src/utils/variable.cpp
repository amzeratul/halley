#include "halley/utils/variable.h"
#include "halley/core/resources/resource_collection.h"
#include "halley/bytes/byte_serializer.h"
using namespace Halley;

Internal::VariableStorage::VariableStorage() noexcept
	: intValue(0)
{
}

Internal::VariableStorage::VariableStorage(const VariableStorage& other) noexcept
{
	*this = other;
}

Internal::VariableStorage::VariableStorage(VariableStorage&& other) noexcept
{
	*this = std::move(other);
}

Internal::VariableStorage& Internal::VariableStorage::operator=(const VariableStorage& other) noexcept
{
	clear();
	
	switch (other.type) {
	case VariableType::Int:
		intValue = other.intValue;
		break;
	case VariableType::Float:
		floatValue = other.floatValue;
		break;
	case VariableType::Int2:
		vector2iValue = other.vector2iValue;
		break;
	case VariableType::Float2:
		vector2fValue = other.vector2fValue;
		break;
	case VariableType::Colour:
		colourValue = other.colourValue;
		break;
	case VariableType::String:
		stringValue = new String(*other.stringValue);
		break;
	}
	type = other.type;
	return *this;
}

Internal::VariableStorage& Internal::VariableStorage::operator=(VariableStorage&& other) noexcept
{
	clear();
	
	switch (other.type) {
	case VariableType::Int:
		intValue = other.intValue;
		break;
	case VariableType::Float:
		floatValue = other.floatValue;
		break;
	case VariableType::Int2:
		vector2iValue = other.vector2iValue;
		break;
	case VariableType::Float2:
		vector2fValue = other.vector2fValue;
		break;
	case VariableType::Colour:
		colourValue = other.colourValue;
		break;
	case VariableType::String:
		stringValue = other.stringValue;
		other.stringValue = nullptr;
		break;
	}
	type = other.type;
	return *this;
}

Internal::VariableStorage::~VariableStorage()
{
	clear();
}

void Internal::VariableStorage::setValue(const ConfigNode& node)
{
	clear();
	
	switch (node.getType()) {
	case ConfigNodeType::Int:
		type = VariableType::Int;
		intValue = node.asInt();
		break;
	case ConfigNodeType::Float:
		type = VariableType::Float;
		floatValue = node.asFloat();
		break;
	case ConfigNodeType::Int2:
		type = VariableType::Int2;
		vector2iValue = node.asVector2i();
		break;
	case ConfigNodeType::Float2:
		type = VariableType::Float2;
		vector2fValue = node.asVector2f();
		break;
	case ConfigNodeType::String:
	{
		const auto strValue = node.asString();
		if (strValue == "true" || strValue == "false") {
			type = VariableType::Int;
			intValue = strValue == "true";
		} else if (strValue.startsWith("#")) {
			type = VariableType::Colour;
			colourValue = Colour4f::fromString(strValue);
		} else {
			type = VariableType::String;
			stringValue = new String(strValue);
		}
		break;
	}
	default:
		throw Exception("Cannot load variable from ConfigNode type " + node.getType(), HalleyExceptions::Utils);
	}
}

void Internal::VariableStorage::serialize(Serializer& s) const
{
	s << type;
	switch (type) {
	case VariableType::Int:
		s << intValue;
		break;
	case VariableType::Float:
		s << floatValue;
		break;
	case VariableType::Int2:
		s << vector2iValue;
		break;
	case VariableType::Float2:
		s << vector2fValue;
		break;
	case VariableType::Colour:
		s << colourValue;
		break;
	case VariableType::String:
		s << *stringValue;
		break;
	default:
		throw Exception("Unknown variable type " + type, HalleyExceptions::Utils);
	}
}

void Internal::VariableStorage::deserialize(Deserializer& s)
{
	clear();
	
	s >> type;
	switch (type) {
	case VariableType::Int:
		s >> intValue;
		break;
	case VariableType::Float:
		s >> floatValue;
		break;
	case VariableType::Int2:
		s >> vector2iValue;
		break;
	case VariableType::Float2:
		s >> vector2fValue;
		break;
	case VariableType::Colour:
		s >> colourValue;
		break;
	case VariableType::String:
		stringValue = new String();
		s >> *stringValue;
	default:
		throw Exception("Unknown variable type " + type, HalleyExceptions::Utils);
	}
}

void Internal::VariableStorage::clear()
{
	if (type == VariableType::String) {
		delete stringValue;
		stringValue = nullptr;
	}
	type = VariableType::Undefined;
}

Internal::VariableBase::VariableBase()
{
}

Internal::VariableBase::VariableBase(const VariableTable& parent, String key)
	: parent(&parent)
	, key(std::move(key))
{
	refresh();
}

void Internal::VariableBase::refresh()
{
	Expects(parent);
	if (parent->getAssetVersion() != parentVersion) {
		parentVersion = parent->getAssetVersion();
		storage = parent->getRawStorage(key);
	}
}

VariableTable::VariableTable()
{
}

VariableTable::VariableTable(const ConfigNode& node)
{
	const auto& entries = node.asMap();
	variables.reserve(entries.size());
	for (auto& kv: entries) {
		Internal::VariableStorage storage;
		storage.setValue(kv.second);
		variables[kv.first] = storage;
	}
}

void VariableTable::serialize(Serializer& s) const
{
	s << variables;
}

void VariableTable::deserialize(Deserializer& s)
{
	s >> variables;
}

std::unique_ptr<VariableTable> VariableTable::loadResource(ResourceLoader& loader)
{
	auto result = std::make_unique<VariableTable>();
	Deserializer::fromBytes(*result, loader.getStatic()->getSpan());
	return result;
}

void VariableTable::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<VariableTable&>(resource));
}

const Internal::VariableStorage& VariableTable::getRawStorage(const String& key) const
{
	auto iter = variables.find(key);
	if (iter == variables.end()) {
		throw Exception("Unknown variable: " + key, HalleyExceptions::Utils);
	}
	return iter->second;
}

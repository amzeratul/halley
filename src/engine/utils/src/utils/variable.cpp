#include "halley/utils/variable.h"
#include "halley/core/resources/resource_collection.h"
#include "halley/bytes/byte_serializer.h"
using namespace Halley;

Internal::VariableStorage::VariableStorage()
	: intValue(0)
{
}

Internal::VariableStorage::VariableStorage(const VariableStorage& other)
{
	*this = other;
}

Internal::VariableStorage& Internal::VariableStorage::operator=(const VariableStorage& other)
{
	switch (other.type) {
	case ConfigNodeType::Int:
		intValue = other.intValue;
		break;
	case ConfigNodeType::Float:
		floatValue = other.floatValue;
		break;
	case ConfigNodeType::Int2:
		vector2iValue = other.vector2iValue;
		break;
	case ConfigNodeType::Float2:
		vector2fValue = other.vector2fValue;
		break;
	}
	type = other.type;
	return *this;
}

void Internal::VariableStorage::setValue(const ConfigNode& node)
{
	type = node.getType();
	switch (node.getType()) {
	case ConfigNodeType::Int:
		intValue = node.asInt();
		break;
	case ConfigNodeType::Float:
		floatValue = node.asFloat();
		break;
	case ConfigNodeType::Int2:
		vector2iValue = node.asVector2i();
		break;
	case ConfigNodeType::Float2:
		vector2fValue = node.asVector2f();
		break;
	case ConfigNodeType::String:
	{
		auto strValue = node.asString();
		if (strValue == "true" || strValue == "false") {
			type = ConfigNodeType::Int;
			intValue = strValue == "true";
		}
	}
	default:
		throw Exception("Cannot load variable from ConfigNode type " + node.getType(), HalleyExceptions::Utils);
	}
}

void Internal::VariableStorage::serialize(Serializer& s) const
{
	s << type;
	switch (type) {
	case ConfigNodeType::Int:
		s << intValue;
		break;
	case ConfigNodeType::Float:
		s << floatValue;
		break;
	case ConfigNodeType::Int2:
		s << vector2iValue;
		break;
	case ConfigNodeType::Float2:
		s << vector2fValue;
		break;
	}
}

void Internal::VariableStorage::deserialize(Deserializer& s)
{
	s >> type;
	switch (type) {
	case ConfigNodeType::Int:
		s >> intValue;
		break;
	case ConfigNodeType::Float:
		s >> floatValue;
		break;
	case ConfigNodeType::Int2:
		s >> vector2iValue;
		break;
	case ConfigNodeType::Float2:
		s >> vector2fValue;
		break;
	}
}

Internal::VariableBase::VariableBase()
{
}

Internal::VariableBase::VariableBase(VariableTable& parent, String key)
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

const Internal::VariableStorage& VariableTable::getRawStorage(const String& key)
{
	auto iter = variables.find(key);
	if (iter == variables.end()) {
		throw Exception("Unknown variable: " + key, HalleyExceptions::Utils);
	}
	return iter->second;
}

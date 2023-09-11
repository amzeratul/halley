#include "halley/utils/variable.h"
#include "halley/resources/resource_collection.h"
#include "halley/bytes/byte_serializer.h"
using namespace Halley;

Internal::VariableBase::VariableBase(const VariableTable& parent, String key)
	: parent(&parent)
	, key(std::move(key))
{
	refresh();
}

void Internal::VariableBase::refresh() const
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
		variables[kv.first] = ConfigNode(kv.second);
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

const ConfigNode& VariableTable::getRawStorage(const String& key) const
{
	if (const auto iter = variables.find(key); iter != variables.end()) {
		return iter->second;
	}
	Logger::logError("Unknown variable: " + key, true);
	return dummy;
}

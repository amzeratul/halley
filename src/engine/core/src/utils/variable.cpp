#include "halley/utils/variable.h"
#include "halley/resources/resource_collection.h"
#include "halley/bytes/byte_serializer.h"
using namespace Halley;

Internal::VariableBase::VariableBase(const VariableTable& parent, std::string_view key)
#ifdef DEV_BUILD
	: parent(&parent)
	, key(key)
#endif
{
}

const ConfigNode& Internal::VariableBase::getValue(const VariableTable& parent, std::string_view key)
{
	return parent.getRawStorage(key);
}

bool Internal::VariableBase::needsRefresh() const
{
#ifdef DEV_BUILD
	HalleyAssertDebug(parent);
	const bool version = parent->getAssetVersion();
	if (parent->getAssetVersion() != parentVersion) [[unlikely]] {
		parentVersion = version;
		return true;
	}
#endif
	return false;
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

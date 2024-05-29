#include "halley/entity/entity_id.h"

#include "halley/entity/entity_factory.h"
#include "halley/entity/world.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/text/string_converter.h"
#include "halley/text/halleystring.h"
#include "halley/bytes/config_node_serializer.h"

using namespace Halley;

EntityId::EntityId(const String& str)
{
	value = str.toInteger64();
}

namespace {
	EntityId fromConfigNode(const ConfigNode& node, const EntitySerializationContext& context)
	{
		if (node.getType() == ConfigNodeType::EntityId) {
			return node.asEntityId();
		}

		if (node.getType() == ConfigNodeType::Del || node.getType() == ConfigNodeType::Undefined) {
			return {};
		}

		const auto& value = node.asString("");
		const auto uuid = value.isEmpty() ? UUID() : UUID(value);
		if (!context.entityContext || !uuid.isValid()) {
			return EntityId();
		}
		return context.entityContext->getEntityIdFromUUID(uuid);
	}
}

EntityId::EntityId(const ConfigNode& node, const EntitySerializationContext& context)
{
	*this = fromConfigNode(node, context);
}

ConfigNode EntityId::toConfigNode(const EntitySerializationContext& context) const
{
	if (!context.entityContext) {
		return ConfigNode(*this);
	}

	if (!isValid()) {
		return ConfigNode(String());
	}

	return ConfigNode(context.entityContext->getUUIDFromEntityId(*this).toString());
}

String EntityId::toString() const
{
	return Halley::toString(value);
}

void EntityId::serialize(Serializer& s) const
{
	if (auto* world = s.getOptions().world) {
		if (const auto e = world->tryGetEntity(*this); e.isValid()) {
			s << e.getInstanceUUID();
		} else {
			s << UUID();
		}
	} else {
		throw Exception("Serializing EntityID requires World to be set in SerializationOptions.", HalleyExceptions::Entity);
	}
}

void EntityId::deserialize(Deserializer& s)
{
	auto* world = s.getOptions().world;
	if (world) {
		UUID uuid;
		s >> uuid;
		
		auto e = world->findEntity(uuid);
		if (e) {
			*this = e->getEntityId();
		} else {
			*this = EntityId();
		}
	} else {
		throw Exception("Deserializing EntityID requires World to be set in SerializationOptions.", HalleyExceptions::Entity);
	}
}

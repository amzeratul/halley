#include "halley/entity/system_message.h"

using namespace Halley;

SystemMessageDestination::SystemMessageDestination(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map) {
		type = node["type"].asEnum<SystemMessageDestinationType>();
		dstPeerId = node["dstPeerId"].asInt(0);
	} else {
		type = node.asEnum(AllClients);
	}
}

void SystemMessageDestination::serialize(Serializer& s) const
{
	s << static_cast<int>(type);
	if (type == SpecificPeer) {
		s << dstPeerId;
	}
}

void SystemMessageDestination::deserialize(Deserializer& s)
{
	int i;
	s >> i;
	type = static_cast<SystemMessageDestinationType>(i);
	if (type == SpecificPeer) {
		s >> dstPeerId;
	} else {
		dstPeerId = 0;
	}
}

String SystemMessageDestination::toString() const
{
	if (type == SpecificPeer) {
		return Halley::toString(type) + "(" + static_cast<int>(dstPeerId) + ")";
	} else {
		return Halley::toString(type);
	}
}

ConfigNode SystemMessageDestination::toConfigNode() const
{
	ConfigNode result;
	if (type == SpecificPeer) {
		result["type"] = type;
		result["dstPeerId"] = dstPeerId;
	} else {
		result = type;
	}
	return result;
}

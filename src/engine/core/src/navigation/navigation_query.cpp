#include "halley/navigation/navigation_query.h"

#include "halley/bytes/config_node_serializer.h"
#include "halley/data_structures/config_node.h"
#include "halley/maths/vector2.h"

using namespace Halley;

NavigationQuery::DebugData::DebugData(String agentId)
	: agentId(std::move(agentId))
{
}

String NavigationQuery::DebugData::toString() const
{
	return "(" + agentId + ")";
}

NavigationQuery::NavigationQuery() = default;

NavigationQuery::NavigationQuery(WorldPosition from, WorldPosition to, PostProcessingType postProcessing, QuantizationType quantizationType, DebugData debugData)
	: from(from)
	, to(to)
	, postProcessingType(postProcessing)
	, quantizationType(quantizationType)
	, debugData(std::move(debugData))
{
}

NavigationQuery::NavigationQuery(const ConfigNode& node)
{
	from = WorldPosition(node["from"]);
	to = WorldPosition(node["to"]);
	postProcessingType = fromString<PostProcessingType>(node["postProcessingType"].asString());
	quantizationType = fromString<QuantizationType>(node["quantizationType"].asString());
}

ConfigNode NavigationQuery::toConfigNode() const
{
	ConfigNode::MapType result;

	result["from"] = from;
	result["to"] = to;
	result["postProcessingType"] = Halley::toString(postProcessingType);
	result["quantizationType"] = Halley::toString(quantizationType);
	
	return result;
}

void NavigationQuery::feedToHasher(Hash::Hasher& hasher) const
{
	ConfigNodeHelper<decltype(from)>::hash(from, hasher);
	ConfigNodeHelper<decltype(to)>::hash(to, hasher);
	ConfigNodeHelper<decltype(postProcessingType)>::hash(postProcessingType, hasher);
	ConfigNodeHelper<decltype(quantizationType)>::hash(quantizationType, hasher);
}

String NavigationQuery::toString() const
{
	using Halley::toString;
	return "navQuery(" + toString(from) + " -> " + toString(to) + ", " + toString(postProcessingType) + ", " + toString(quantizationType) + ")";
}

bool NavigationQuery::operator==(const NavigationQuery& other) const
{
	return from == other.from
		&& to == other.to
		&& postProcessingType == other.postProcessingType
		&& quantizationType == other.quantizationType;
}

bool NavigationQuery::operator!=(const NavigationQuery& other) const
{
	return !(*this == other);
}

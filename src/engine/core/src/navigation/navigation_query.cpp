#include "halley/navigation/navigation_query.h"
#include "halley/data_structures/config_node.h"
#include "halley/maths/vector2.h"

using namespace Halley;

NavigationQuery::DebugData::DebugData(String agentId)
	: agentId(std::move(agentId))
{
}

NavigationQuery::DebugData::DebugData(const ConfigNode& data)
{
	agentId = data["agentId"].asString("");
}

String NavigationQuery::DebugData::toString() const
{
	return "(" + agentId + ")";
}

ConfigNode NavigationQuery::DebugData::toConfigNode() const
{
	ConfigNode::MapType result;
	result["agentId"] = agentId;
	return result;
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
	debugData = DebugData(node["debugData"]);
}

ConfigNode NavigationQuery::toConfigNode() const
{
	ConfigNode::MapType result;

	result["from"] = from;
	result["to"] = to;
	result["postProcessingType"] = Halley::toString(postProcessingType);
	result["quantizationType"] = Halley::toString(quantizationType);
	result["debugData"] = debugData.toConfigNode();
	
	return result;
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

#include "halley/navigation/navigation_query.h"
#include "halley/data_structures/config_node.h"
#include "halley/maths/vector2.h"

using namespace Halley;

NavigationQuery::NavigationQuery() = default;

NavigationQuery::NavigationQuery(WorldPosition from, WorldPosition to, PostProcessingType postProcessing, QuantizationType quantizationType)
	: from(from)
	, to(to)
	, postProcessingType(postProcessing)
	, quantizationType(quantizationType)
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

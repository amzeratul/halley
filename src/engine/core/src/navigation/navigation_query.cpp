#include "halley/navigation/navigation_query.h"
#include "halley/data_structures/config_node.h"
#include "halley/maths/vector2.h"

using namespace Halley;

NavigationQuery::NavigationQuery() = default;

NavigationQuery::NavigationQuery(WorldPosition from, WorldPosition to, PostProcessingType postProcessing)
	: from(from)
	, to(to)
	, postProcessingType(postProcessing)
{
}

NavigationQuery::NavigationQuery(const ConfigNode& node)
{
	from = node["from"].asVector2f();
	to = node["to"].asVector2f();
	postProcessingType = fromString<PostProcessingType>(node["postProcessingType"].asString());
}

ConfigNode NavigationQuery::toConfigNode() const
{
	ConfigNode::MapType result;

	result["from"] = from;
	result["to"] = to;
	result["postProcessingType"] = Halley::toString(postProcessingType);
	
	return result;
}

String NavigationQuery::toString() const
{
	using Halley::toString;
	return "navQuery(" + toString(from) + " -> " + toString(to) + ", " + toString(postProcessingType) + ")";
}

bool NavigationQuery::operator==(const NavigationQuery& other) const
{
	return from == other.from
		&& to == other.to
		&& postProcessingType == other.postProcessingType;
}

bool NavigationQuery::operator!=(const NavigationQuery& other) const
{
	return !(*this == other);
}

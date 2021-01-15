#include "halley/navigation/navigation_query.h"
#include "halley/data_structures/config_node.h"
#include "halley/maths/vector2.h"

using namespace Halley;

NavigationQuery::NavigationQuery() = default;

NavigationQuery::NavigationQuery(Vector2f from, int fromSubWorld, Vector2f to, int toSubWorld, PostProcessingType postProcessing)
	: from(from)
	, fromSubWorld(fromSubWorld)
	, to(to)
	, toSubWorld(toSubWorld)
	, postProcessingType(postProcessing)
{
}

NavigationQuery::NavigationQuery(const ConfigNode& node)
{
	from = node["from"].asVector2f();
	fromSubWorld = node["fromSubWorld"].asInt();
	to = node["to"].asVector2f();
	toSubWorld = node["toSubWorld"].asInt();
	postProcessingType = fromString<PostProcessingType>(node["postProcessingType"].asString());
}

ConfigNode NavigationQuery::toConfigNode() const
{
	ConfigNode::MapType result;

	result["from"] = from;
	result["fromSubWorld"] = fromSubWorld;
	result["to"] = to;
	result["toSubWorld"] = toSubWorld;
	result["postProcessingType"] = toString(postProcessingType);
	
	return result;
}

bool NavigationQuery::operator==(const NavigationQuery& other) const
{
	return from == other.from
		&& fromSubWorld == other.fromSubWorld
		&& to == other.to
		&& toSubWorld == other.toSubWorld
		&& postProcessingType == other.postProcessingType;
}

bool NavigationQuery::operator!=(const NavigationQuery& other) const
{
	return !(*this == other);
}

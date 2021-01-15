#include "halley/navigation/navigation_path.h"
using namespace Halley;

NavigationPath::NavigationPath() = default;

NavigationPath::NavigationPath(const NavigationQuery& query, std::vector<Vector2f> path, std::vector<RegionNode> regions)
	: path(std::move(path))
	, regions(std::move(regions))
	, query(query)
{
}

NavigationPath::NavigationPath(const ConfigNode& node)
{
	query = NavigationQuery(node["query"]);
}

ConfigNode NavigationPath::toConfigNode() const
{
	ConfigNode::MapType result;

	result["query"] = query.toConfigNode();
	
	return result;
}

bool NavigationPath::operator==(const NavigationPath& other) const
{
	return query == other.query;
}

bool NavigationPath::operator!=(const NavigationPath& other) const
{
	return !(*this == other);
}

ConfigNode ConfigNodeSerializer<NavigationPath>::serialize(const NavigationPath& path, const ConfigNodeSerializationContext& context)
{
	return path.toConfigNode();
}

NavigationPath ConfigNodeSerializer<NavigationPath>::deserialize(const ConfigNodeSerializationContext& context,	const ConfigNode& node)
{
	return NavigationPath(node);
}

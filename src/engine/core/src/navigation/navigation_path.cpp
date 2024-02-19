#include "halley/navigation/navigation_path.h"
using namespace Halley;

NavigationPath::NavigationPath() = default;

NavigationPath::NavigationPath(const NavigationQuery& query, Vector<Vector2f> path, Vector<RegionNode> regions)
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

NavigationPath NavigationPath::merge(gsl::span<const NavigationPath> paths)
{
	if (paths.empty()) {
		return {};
	}

	NavigationPath result;
	result.query = paths.front().query;
	result.query.to = paths.back().query.to;

	for (const auto& path: paths) {
		for (const auto p: path.path) {
			if (result.path.empty() || p != result.path.back()) {
				result.path.push_back(p);
			}
		}
	}

	result.regions = paths.back().regions;

	return result;
}

ConfigNode ConfigNodeSerializer<NavigationPath>::serialize(const NavigationPath& path, const EntitySerializationContext& context)
{
	return path.toConfigNode();
}

NavigationPath ConfigNodeSerializer<NavigationPath>::deserialize(const EntitySerializationContext& context,	const ConfigNode& node)
{
	return NavigationPath(node);
}

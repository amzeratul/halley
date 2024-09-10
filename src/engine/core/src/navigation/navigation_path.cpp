#include "halley/navigation/navigation_path.h"
using namespace Halley;

NavigationPath::NavigationPath() = default;

NavigationPath::NavigationPath(NavigationQuery query, Vector<Point> path)
	: path(std::move(path))
	, query(std::move(query))
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

float NavigationPath::getLength() const
{
	float len = 0;
	const auto ps = path.span();
	for (size_t i = 1; i < ps.size(); ++i) {
		len += (ps[i].pos.pos - ps[i - 1].pos.pos).length();
	}
	return len;
}

NavigationPath NavigationPath::merge(gsl::span<const NavigationPath> paths)
{
	if (paths.empty()) {
		return {};
	}

	NavigationPath result = paths.front();

	for (size_t i = 1; i < paths.size(); ++i) {
		auto& other = paths[i];
		result.path.insert(result.path.end(), other.path.begin(), other.path.end());
	}

	result.query.to = paths.back().query.to;

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

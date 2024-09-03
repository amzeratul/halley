#pragma once

#include "halley/data_structures/vector.h"
#include "halley/bytes/config_node_serializer.h"
#include "navigation_query.h"

namespace Halley {
	class NavigationPath {
	public:
		// The path is defined by a sequence of points, followed by additional paths
		Vector<WorldPosition> path;
		Vector<NavigationPath> followUpPaths;

		NavigationQuery query;

		NavigationPath();
		NavigationPath(NavigationQuery query, Vector<WorldPosition> path);
		explicit NavigationPath(const ConfigNode& node);

		ConfigNode toConfigNode() const;

		bool operator==(const NavigationPath& other) const;
		bool operator!=(const NavigationPath& other) const;

		static NavigationPath merge(gsl::span<const NavigationPath> paths);
	};

	template<>
	class ConfigNodeSerializer<NavigationPath> {
	public:
		ConfigNode serialize(const NavigationPath& path, const EntitySerializationContext& context);
		NavigationPath deserialize(const EntitySerializationContext& context, const ConfigNode& node);
	};
}

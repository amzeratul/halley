#pragma once

#include "halley/data_structures/vector.h"
#include "halley/bytes/config_node_serializer.h"
#include "navigation_query.h"

namespace Halley {
	class NavigationPath {
	public:
		struct Point {
			WorldPosition pos;
			uint16_t navmeshId;

			Point() = default;
			Point(WorldPosition pos, uint16_t navmeshId = std::numeric_limits<uint16_t>::max())
				: pos(pos)
				, navmeshId(navmeshId)
			{}
			Point(Vector2f pos, int subWorld, uint16_t navmeshId = std::numeric_limits<uint16_t>::max())
				: pos(pos, subWorld)
				, navmeshId(navmeshId)
			{}
		};

		Vector<Point> path;
		NavigationQuery query;

		NavigationPath();
		NavigationPath(NavigationQuery query, Vector<Point> path);
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

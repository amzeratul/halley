#pragma once

#include <cstdint>
#include <limits>
#include "halley/data_structures/vector.h"
#include "halley/file_formats/config_file.h"
#include "halley/bytes/config_node_serializer.h"
#include "navigation_query.h"

namespace Halley {
	class NavigationPath {
	public:
		struct RegionNode {
			uint16_t regionNodeId;
			uint16_t exitEdgeId;

			RegionNode(uint16_t regionNodeId = 0, uint16_t exitEdgeId = std::numeric_limits<uint16_t>::max()) : regionNodeId(regionNodeId), exitEdgeId(exitEdgeId) {}
		};
		
		// The path is defined by a sequence of points, followed by a sequence of regions that need further processing, followed by additional paths
		Vector<Vector2f> path;
		Vector<RegionNode> regions;
		Vector<NavigationPath> followUpPaths;

		NavigationQuery query;

		NavigationPath();
		NavigationPath(const NavigationQuery& query, Vector<Vector2f> path, Vector<RegionNode> regions = {});
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

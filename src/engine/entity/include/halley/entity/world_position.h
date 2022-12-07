#pragma once
#include "halley/data_structures/config_node.h"
#include "halley/maths/vector2.h"

namespace Halley {
	class WorldPosition {
	public:
		Vector2f pos;
		int subWorld;
		
		constexpr WorldPosition(Vector2f pos = {}, int subWorld = 0)
			: pos(pos)
			, subWorld(subWorld)
		{
		}

		WorldPosition(const ConfigNode& node, Vector2f defaultPos = {}, int defaultSubWorld = 0);
		ConfigNode toConfigNode() const;
		String toString() const;
	};
}

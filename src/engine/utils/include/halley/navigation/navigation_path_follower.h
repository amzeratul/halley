#pragma once

#include "navigation_path.h"

namespace Halley {
	class NavmeshSet;

	class NavigationPathFollower {
	public:
		NavigationPathFollower() = default;
		explicit NavigationPathFollower(const ConfigNode& node);

		ConfigNode toConfigNode() const;
		
		void setPath(std::optional<NavigationPath> p);
		const std::optional<NavigationPath>& getPath() const;

		void update(Vector2f curPos, int curSubWorld, const NavmeshSet& navmeshSet, float threshold);
		
		Vector2f getNextPosition() const;
		size_t getNextPathIdx() const;
		bool isDone() const;

	private:
		Vector2f curPos;
		int curSubWorld = 0;
		size_t nextPathIdx = 0;
		size_t nextRegionIdx = 0;
		std::optional<NavigationPath> path;
		bool needsToReEvaluatePath = false;

		void goToNextRegion(const NavmeshSet& navmeshSet);
		void reEvaluatePath(const NavmeshSet& navmeshSet);
	};

	template<>
	class ConfigNodeSerializer<NavigationPathFollower> {
	public:
		ConfigNode serialize(const NavigationPathFollower& follower, const ConfigNodeSerializationContext& context);
		NavigationPathFollower deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node);
	};
}

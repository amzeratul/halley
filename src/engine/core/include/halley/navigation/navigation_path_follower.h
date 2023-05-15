#pragma once

#include "navigation_path.h"
#include "world_position.h"

namespace Halley {
	class NavmeshSet;

	class NavigationPathFollower {
	public:
		NavigationPathFollower() = default;
		explicit NavigationPathFollower(const ConfigNode& node);

		ConfigNode toConfigNode() const;
		
		void setPath(std::optional<NavigationPath> p);
		const std::optional<NavigationPath>& getPath() const;
		gsl::span<const Vector2f> getNextPathPoints() const;

		void update(WorldPosition curPos, const NavmeshSet& navmeshSet, float threshold);
		
		Vector2f getNextPosition() const;
		size_t getNextPathIdx() const;
		uint16_t getCurrentRegionId() const;
		bool isDone() const;
		int getNavmeshSubWorld() const;

	private:
		WorldPosition curPos;
		size_t nextPathIdx = 0;
		size_t nextRegionIdx = 0;
		std::optional<NavigationPath> path;
		bool needsToReEvaluatePath = false;
		int navmeshSubWorld = 0;

		void goToNextRegion(const NavmeshSet& navmeshSet);
		void reEvaluatePath(const NavmeshSet& navmeshSet);
	};

	template<>
	class ConfigNodeSerializer<NavigationPathFollower> {
	public:
		ConfigNode serialize(const NavigationPathFollower& follower, const EntitySerializationContext& context);
		NavigationPathFollower deserialize(const EntitySerializationContext& context, const ConfigNode& node);
	};
}

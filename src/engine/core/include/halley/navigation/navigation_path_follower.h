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

		void setComputingPath();
		void setPath(std::optional<NavigationPath> p, ConfigNode params = {});
		const std::optional<NavigationPath>& getPath() const;
		gsl::span<const WorldPosition> getNextPathPoints() const;

		void update(WorldPosition curPos, const NavmeshSet& navmeshSet, float threshold);
		
		WorldPosition getNextPosition() const;
		WorldPosition getCurPosition() const;

		size_t getNextPathIdx() const;
		bool isFollowingPath() const;
		bool isDone() const;
		void setAllPathSubWorld(int value);
		int getNavmeshSubWorld() const;

		const ConfigNode& getParams() const;
		ConfigNode& getParams();

	private:
		WorldPosition curPos;
		size_t nextPathIdx = 0;
		std::optional<NavigationPath> path;
		bool needsToReEvaluatePath = false;
		bool computingPath = false;
		ConfigNode params;

		void nextSubPath();
		void doSetPath(std::optional<NavigationPath> p);
		void reEvaluatePath(const NavmeshSet& navmeshSet);
	};

	template<>
	class ConfigNodeSerializer<NavigationPathFollower> {
	public:
		ConfigNode serialize(const NavigationPathFollower& follower, const EntitySerializationContext& context);
		NavigationPathFollower deserialize(const EntitySerializationContext& context, const ConfigNode& node);
	};
}

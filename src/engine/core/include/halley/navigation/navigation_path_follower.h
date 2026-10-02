#pragma once

#include "navigation_path.h"
#include "world_position.h"

namespace Halley {
	class NavmeshSet;

	class NavigationPathParams {
	public:
		NavigationPathParams() = default;
		explicit NavigationPathParams(const ConfigNode& node);

		ConfigNode toConfigNode() const;

		bool run = false;
		bool backwards = false;
		bool manualPath = false;
		float speed = 1.0f;
		OptionalLite<int> faceAfter;
	};

	class NavigationPathFollower {
	public:
		NavigationPathFollower() = default;
		explicit NavigationPathFollower(const ConfigNode& node);

		ConfigNode toConfigNode() const;
		void feedToHasher(Hash::Hasher& hasher) const;

		void setComputingPath();
		void clear();
		void setPath(std::optional<NavigationPath> p, NavigationPathParams params = {});
		const std::optional<NavigationPath>& getPath() const;
		gsl::span<const NavigationPath::Point> getNextPathPoints() const;

		void update(WorldPosition curPos, const NavmeshSet& navmeshSet, float minThreshold, float maxThreshold);
		
		WorldPosition getNextPosition() const;
		WorldPosition getPointAtIdx(size_t idx) const;

		size_t getNextPathIdx() const;
		bool isFollowingPath() const;
		bool isDone() const;
		void detachFromNavmesh();

		float getDistanceLeft(float anisotropy) const;

		const NavigationPathParams& getParams() const;
		NavigationPathParams& getParams();

	private:
		WorldPosition curPos;
		size_t nextPathIdx = 0;
		std::optional<NavigationPath> path;
		mutable std::optional<float> distPathCache;
		bool needsToReEvaluatePath = false;
		bool computingPath = false;
		NavigationPathParams params;

		void nextSubPath();
		void doSetPath(std::optional<NavigationPath> p);
		void reEvaluatePath(const NavmeshSet& navmeshSet);
	};

	template<>
	class ConfigNodeSerializer<NavigationPathFollower> {
	public:
		ConfigNode serialize(const NavigationPathFollower& follower, const EntitySerializationContext& context);
		NavigationPathFollower deserialize(const EntitySerializationContext& context, const ConfigNode& node);
		void hash(const NavigationPathFollower& follower, Hash::Hasher& hasher);
	};
}

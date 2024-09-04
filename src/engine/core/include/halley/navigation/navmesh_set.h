#pragma once

#include "navmesh.h"
#include "navigation_query.h"
#include "navigation_path.h"

namespace Halley {
	class NavmeshSet : public Resource {
	public:
		NavmeshSet();
		NavmeshSet(const ConfigNode& nodeData);

		ConfigNode toConfigNode() const;

		static std::shared_ptr<NavmeshSet> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::NavmeshSet; }

		void reload(Resource&& resource) override;
		void makeDefault();

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void add(Navmesh navmesh);
		void addChunk(NavmeshSet navmeshSet, Vector2f origin, Vector2i gridPosition);
		void addRaw(NavmeshSet navmeshSet);
		void clear();
		void clearSubWorld(int subWorld);

		void linkNavmeshes();
		void reportUnlinkedPortals(std::function<String(Vector2i)> getChunkName) const;
		void setMaxDistancesToNavmesh(float startDistance, float endDistance);

		std::optional<NavigationPath> pathfind(const NavigationQuery& query, String* errorOut = nullptr, float anisotropy = 1.0f, float nudge = 0.1f) const;
		std::optional<NavigationPath> pathfindInRegion(const NavigationQuery& query, uint16_t regionId) const;

		gsl::span<const Navmesh> getNavmeshes() const { return navmeshes; }
		const Navmesh* getNavMeshAt(WorldPosition pos) const;
		size_t getNavMeshIdxAt(WorldPosition pos) const;
		std::pair<size_t, WorldPosition> getNavMeshIdxAtWithTolerance(WorldPosition pos, float maxDist = std::numeric_limits<float>::infinity(), float anisotropy = 1.0f, float nudge = 0.1f) const;
		std::optional<WorldPosition> getClosestPointTo(WorldPosition pos, float maxDist = std::numeric_limits<float>::infinity(), float anisotropy = 1.0f, float nudge = 0.1f, bool anySubWorld = false) const;

		std::pair<uint16_t, uint16_t> getPortalDestination(uint16_t region, uint16_t edge) const;

	private:
		struct PortalConnection {
			uint16_t portalId;
			uint16_t regionId;
			float cost;

			PortalConnection() = default;
			PortalConnection(uint16_t portalId, uint16_t regionId, float cost)
				: portalId(portalId)
				, regionId(regionId)
				, cost(cost)
			{}
		};
		
		struct PortalNode {
			Vector2f pos;
			Vector<PortalConnection> connections;
			uint16_t fromRegion;
			uint16_t fromPortal;
			uint16_t toRegion;
			uint16_t toPortal;

			PortalNode() = default;
			PortalNode(Vector2f pos, uint16_t fromRegion, uint16_t fromPortal, uint16_t toRegion, uint16_t toPortal)
				: pos(pos), fromRegion(fromRegion), fromPortal(fromPortal), toRegion(toRegion), toPortal(toPortal)
			{}
		};

		struct RegionNode {
			Vector<uint16_t> portals;
		};

		struct NodeAndConn {
			uint16_t regionNodeId;
			uint16_t exitEdgeId;

			NodeAndConn(uint16_t regionNodeId = 0, uint16_t exitEdgeId = std::numeric_limits<uint16_t>::max()) : regionNodeId(regionNodeId), exitEdgeId(exitEdgeId) {}
		};

		using NodeId = uint16_t;

		struct State {
			float gScore = std::numeric_limits<float>::infinity();
			float fScore = std::numeric_limits<float>::infinity();
			NodeId cameFrom;
			bool inOpenSet = false;
			bool inClosedSet = false;
		};

		class NodeComparator {
		public:
			NodeComparator(const Vector<State>& state) : state(state) {}
			
			bool operator()(NodeId a, NodeId b) const
			{
				return state[a].fScore > state[b].fScore;
			}

		private:
			const Vector<State>& state;
		};

		Vector<Navmesh> navmeshes;
		Vector<PortalNode> portalNodes;
		Vector<RegionNode> regionNodes;
		float maxStartDistanceToNavMesh = 10.0f;
		float maxEndDistanceToNavMesh = 1.0f;

		void tryLinkNavMeshes(uint16_t idxA, uint16_t idxB);

		NavigationPath extendToFullPath(const NavigationQuery& query, const Vector<NodeAndConn>& path) const;
		Vector<NodeAndConn> findRegionPath(Vector2f startPos, Vector2f endPos, uint16_t fromRegionId, uint16_t toRegionId) const;

		void postProcessPath(NavigationPath& path) const;
		void simplifyPath(Vector<NavigationPath::Point>& points, NavigationQuery::PostProcessingType type) const;
		void quantizePath(Vector<NavigationPath::Point>& points, NavigationQuery::QuantizationType type) const;
		void quantizePath8Way(Vector<NavigationPath::Point>& points, Vector2f scale) const;
		bool isPathClear(std::initializer_list<const NavigationPath::Point> points) const;
		bool isPathClear(gsl::span<const NavigationPath::Point> points) const;
		std::pair<std::optional<Vector2f>, float> findRayCollision(NavigationPath::Point from, NavigationPath::Point to) const;
		std::pair<std::optional<Vector2f>, float> findRayCollision(NavigationPath::Point from, NavigationPath::Point to, uint16_t startNodeId) const;

		void assignNavmeshIds();
	};
}

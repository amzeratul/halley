#pragma once

#include "navigation_path.h"
#include "navigation_query.h"
#include "halley/maths/polygon.h"
#include "halley/maths/base_transform.h"

namespace Halley {
	class Random;

	struct NavmeshBounds {
		Vector2f origin;
		Vector2f side0;
		Vector2f side1;
		size_t side0Divisions;
		size_t side1Divisions;
		Vector2f scaleFactor;
		Base2D base;

		NavmeshBounds(Vector2f origin, Vector2f side0, Vector2f side1, size_t side0Divisions, size_t side1Divisions, Vector2f scaleFactor);
		std::array<Line, 4> makeEdges() const;
	};

	struct NavmeshSubworldPortal {
		LineSegment segment;
		Vector2f normal;
		int subworldDelta = 0;

		bool operator==(const NavmeshSubworldPortal& other) const;
		bool operator!=(const NavmeshSubworldPortal& other) const;
		bool isBeyondPortal(Vector2f p) const;
	};

	class Navmesh {
	public:
		constexpr static size_t MaxConnections = 8;
		using NodeId = uint16_t;
		
		class PolygonData {
		public:
			Polygon polygon;
			std::vector<int> connections;
			float weight;
		};

		class alignas(64) Node {
		public:
			Vector2f pos;
			size_t nConnections = 0;
			std::array<OptionalLite<NodeId>, MaxConnections> connections;
			std::array<float, MaxConnections> costs;

			Node() = default;
			Node(const ConfigNode& node);

			ConfigNode toConfigNode() const;
		};
		
		struct NodeAndConn {
			NodeId node;
			uint16_t connectionIdx;
			
			NodeAndConn(NodeId node = -1, uint16_t connection = std::numeric_limits<uint16_t>::max()) : node(node), connectionIdx(connection) {}
		};

		struct Portal {
			int id;
			Vector2f pos;
			std::vector<NodeAndConn> connections;
			std::vector<Vector2f> vertices;
			bool connected = false;
			bool regionLink = false;
			bool subWorldLink = false;

			Portal(int id);
			explicit Portal(const ConfigNode& node);

			ConfigNode toConfigNode() const;
			void postProcess(gsl::span<const Polygon> polygons, std::vector<Portal>& dst);
			
			bool canJoinWith(const Portal& other, float epsilon) const;
			void updateLocal();
			void translate(Vector2f offset);
		};
		
		Navmesh();
		Navmesh(const ConfigNode& nodeData);
		Navmesh(std::vector<PolygonData> polygons, const NavmeshBounds& bounds, int subWorld);

		[[nodiscard]] std::optional<std::vector<NodeAndConn>> pathfindNodes(const NavigationQuery& query) const;
		[[nodiscard]] std::optional<NavigationPath> pathfind(const NavigationQuery& query) const;

		[[nodiscard]] ConfigNode toConfigNode() const;
		
		[[nodiscard]] const std::vector<Node>& getNodes() const { return nodes; }
		[[nodiscard]] const std::vector<Polygon>& getPolygons() const { return polygons; }
		[[nodiscard]] const std::vector<Portal>& getPortals() const { return portals; }
		[[nodiscard]] const std::vector<float>& getWeights() const { return weights; }
		[[nodiscard]] const std::vector<std::pair<uint16_t, LineSegment>>& getOpenEdges() const { return openEdges; }
		[[nodiscard]] const Polygon& getPolygon(int id) const;
		[[nodiscard]] size_t getNumNodes() const { return nodes.size(); }
		[[nodiscard]] std::optional<NodeId> getNodeAt(Vector2f position) const;
		[[nodiscard]] bool containsPoint(Vector2f position) const;
		
		// Returns empty if no collision is found (i.e. fully contained within navmesh)
		// Otherwise returns collision point
		[[nodiscard]] std::optional<Vector2f> findRayCollision(Ray ray, float maxDistance) const;

		void setWorldPosition(Vector2f offset, Vector2i worldGridPos);
		[[nodiscard]] Vector2i getWorldGridPos() const { return worldGridPos; }
		[[nodiscard]] int getSubWorld() const { return subWorld; }
		[[nodiscard]] Vector2f getOffset() const { return offset; }

		void markPortalConnected(size_t idx);
		void markPortalsDisconnected();

		float getArea() const;
		Vector2f getRandomPoint(Random& rng) const;

		Base2D getNormalisedCoordinatesBase() const { return normalisedCoordinatesBase; }

	private:
		struct State {
			float gScore = std::numeric_limits<float>::infinity();
			float fScore = std::numeric_limits<float>::infinity();
			NodeAndConn cameFrom;
			bool inOpenSet = false;
			bool inClosedSet = false;
		};

		class NodeComparator {
		public:
			NodeComparator(const std::vector<State>& state) : state(state) {}
			
			bool operator()(Navmesh::NodeId a, Navmesh::NodeId b) const
			{
				return state[a].fScore > state[b].fScore;
			}

		private:
			const std::vector<State>& state;
		};

		std::vector<Node> nodes;
		std::vector<Polygon> polygons;
		std::vector<Portal> portals;
		std::vector<float> weights;
		std::vector<std::pair<uint16_t, LineSegment>> openEdges;
		int subWorld = 0;

		Vector2i gridSize = Vector2i(20, 20);
		std::vector<std::vector<NodeId>> polyGrid; // Quick lookup of polygons

		Vector2f origin;
		Base2D normalisedCoordinatesBase;
		Vector2f scaleFactor; // Use for non-uniform movement, e.g. in isometric when moving left/right is cheaper than up/down (in that case, scaleFactor might be something like (1.0f, 2.0f))

		Vector2f offset;
		Vector2i worldGridPos;

		float totalArea = 0;

		std::optional<std::vector<NodeAndConn>> pathfind(int fromId, int toId) const;
		std::vector<NodeAndConn> makeResult(const std::vector<State>& state, int startId, int endId) const;
		std::optional<NavigationPath> makePath(const NavigationQuery& query, const std::vector<NodeAndConn>& nodePath) const;
		void postProcessPath(std::vector<Vector2f>& points, NavigationQuery::PostProcessingType type) const;

		std::pair<std::optional<Vector2f>, float> findRayCollision(Ray ray, float maxDistance, NodeId initialPolygon) const;

		void processPolygons();
		void addPolygonsToGrid();
		void addPolygonToGrid(const Polygon& poly, NodeId idx);
		gsl::span<const NodeId> getPolygonsAt(Vector2f pos, bool allowOutside) const;

		void addToPortals(NodeAndConn nodeAndConn, int id);
		Portal& getPortals(int id);
		void postProcessPortals();

		void computeArea();

		void generateOpenEdges();
	};
}

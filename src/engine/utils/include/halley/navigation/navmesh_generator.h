#pragma once

#include "navmesh.h"
#include "navmesh_set.h"

namespace Halley {
	class NavmeshGenerator {
	public:
		static NavmeshSet generate(const NavmeshBounds& bounds, gsl::span<const Polygon> obstacles, gsl::span<const Polygon> regions, int subWorld, float agentSize);

	private:
		class NavmeshNode {
		public:
			Polygon polygon;
			std::vector<int> connections;
			int region = -1;
			int regionGroup = 0;
			int remap;
			bool alive = true;
			bool tagged = false;

			NavmeshNode() = default;
			NavmeshNode(Polygon polygon, std::vector<int> connections)
				: polygon(std::move(polygon))
				, connections(std::move(connections))
			{}
		};
		
		constexpr static size_t maxPolygonSides = 8;

		static std::vector<Polygon> generateByPolygonSubtraction(gsl::span<const Polygon> inputPolygons, gsl::span<const Polygon> obstacles, Circle bounds);
		static std::vector<Polygon> preProcessObstacles(gsl::span<const Polygon> obstacles, float agentSize);
		static Polygon makeAgentMask(float agentSize);

		static std::vector<NavmeshNode> toNavmeshNode(std::vector<Polygon> polygons);
		static void generateConnectivity(gsl::span<NavmeshNode> polygons);
		static void postProcessPolygons(std::vector<NavmeshNode>& polygons, float maxSize);
		static void removeDeadPolygons(std::vector<NavmeshNode>& polygons);

		static std::optional<NavmeshNode> merge(const NavmeshNode& a, const NavmeshNode& b, size_t aEdgeIdx, size_t bEdgeIdx, size_t aIdx, size_t bIdx, float maxSize);
		static void remapConnections(NavmeshNode& poly, int from, int to);

		static void limitPolygonSides(std::vector<Polygon>& polygons, size_t maxSides);

		static void applyRegions(gsl::span<NavmeshNode> nodes, gsl::span<const Polygon> regions);
		static int assignRegions(gsl::span<NavmeshNode> nodes);
		static void floodFillRegion(gsl::span<NavmeshNode> nodes, NavmeshNode& firstNode, int regionGroup, int region);

		static std::optional<size_t> getNavmeshEdge(NavmeshNode& node, size_t side, gsl::span<const Line> mapEdges);

		static Navmesh makeNavmesh(gsl::span<NavmeshNode> nodes, const NavmeshBounds& bounds, int region, int subWorld);
	};
}

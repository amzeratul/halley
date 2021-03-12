#include "halley/navigation/navmesh_set.h"

#include "halley/data_structures/priority_queue.h"
#include "halley/support/logger.h"
using namespace Halley;

NavmeshSet::NavmeshSet()
{
}

NavmeshSet::NavmeshSet(const ConfigNode& nodeData)
{
	if (nodeData.getType() == ConfigNodeType::Map) {
		navmeshes = nodeData["navmeshes"].asVector<Navmesh>();
	}
}

ConfigNode NavmeshSet::toConfigNode() const
{
	ConfigNode::MapType result;
	result["navmeshes"] = navmeshes;
	return result;
}

void NavmeshSet::add(Navmesh navmesh)
{
	navmeshes.push_back(std::move(navmesh));
}

void NavmeshSet::addChunk(NavmeshSet navmeshSet, Vector2f origin, Vector2i gridPosition)
{
	for (auto& navmesh: navmeshSet.navmeshes) {
		navmesh.setWorldPosition(origin, gridPosition);
		add(std::move(navmesh));
	}
}

void NavmeshSet::addRaw(NavmeshSet navmeshSet)
{
	for (auto& navmesh: navmeshSet.navmeshes) {
		navmeshes.push_back(std::move(navmesh));
	}
}

void NavmeshSet::clear()
{
	navmeshes.clear();
}

void NavmeshSet::clearSubWorld(int subWorld)
{
	navmeshes.erase(std::remove_if(navmeshes.begin(), navmeshes.end(), [&] (const Navmesh& nav) { return nav.getSubWorld() == subWorld; }), navmeshes.end());
}

std::optional<NavigationPath> NavmeshSet::pathfind(const NavigationQuery& query) const
{
	const size_t fromRegion = getNavMeshIdxAt(query.from, query.fromSubWorld);
	const size_t toRegion = getNavMeshIdxAt(query.to, query.toSubWorld);
	constexpr size_t notFound = std::numeric_limits<size_t>::max();

	if (fromRegion == notFound || toRegion == notFound) {
		// Failed
		return {};
	} else if (fromRegion == toRegion) {
		// Just path in that mesh
		return pathfindInRegion(query, static_cast<uint16_t>(fromRegion));
	} else {
		// Gotta path between regions first
		auto regionPath = findRegionPath(query.from, query.to, static_cast<uint16_t>(fromRegion), static_cast<uint16_t>(toRegion));
		if (regionPath.size() <= 1) {
			// Failed
			return {};
		} else {
			return NavigationPath(query, {}, std::move(regionPath));
		}
	}
}

std::optional<NavigationPath> NavmeshSet::pathfindInRegion(const NavigationQuery& query, uint16_t regionId) const
{
	return navmeshes[regionId].pathfind(query);
}

const Navmesh* NavmeshSet::getNavMeshAt(Vector2f pos, int subWorld) const
{
	for (const auto& navmesh: navmeshes) {
		if (navmesh.getSubWorld() == subWorld && navmesh.containsPoint(pos)) {
			return &navmesh;
		}
	}
	
	return nullptr;
}

size_t NavmeshSet::getNavMeshIdxAt(Vector2f pos, int subWorld) const
{
	size_t i = 0;
	for (const auto& navmesh: navmeshes) {
		if (navmesh.getSubWorld() == subWorld && navmesh.containsPoint(pos)) {
			return i;
		}
		i++;
	}
	
	return std::numeric_limits<size_t>::max();
}

void NavmeshSet::linkNavmeshes()
{
	regionNodes.clear();
	regionNodes.resize(navmeshes.size());
	portalNodes.clear();

	for (auto& navmesh: navmeshes) {
		navmesh.markPortalsDisconnected();
	}

	// Link meshes
	const uint16_t nMeshes = static_cast<uint16_t>(navmeshes.size());
	for (uint16_t i = 0; i < nMeshes; ++i) {
		for (uint16_t j = i + 1; j < nMeshes; ++j) {
			tryLinkNavMeshes(i, j);
		}
	}

	// Generate portal graph
	for (size_t curPortalId = 0; curPortalId < portalNodes.size(); ++curPortalId) {
		auto& portalNode = portalNodes[curPortalId];
		
		// Insert all edges that can be reached by these two regions as neighbours of this edge
		const auto& dstRegion = regionNodes[portalNode.toRegion];
		const auto curPos = portalNode.pos;
		
		portalNode.connections.reserve(dstRegion.portals.size() - 1);
		for (size_t i = 0; i < dstRegion.portals.size(); ++i) {
			if (curPortalId != portalNode.toPortal) {
				const auto dstPortalId = dstRegion.portals[i];
				const auto& other = portalNodes[dstPortalId];
				portalNode.connections.emplace_back(dstPortalId, portalNode.toRegion, (other.pos - curPos).length());
			}
		}
	}
}

void NavmeshSet::reportUnlinkedPortals() const
{
	std::set<Vector2i> occupiedGrids;
	for (const auto& navmesh: navmeshes) {
		occupiedGrids.insert(navmesh.getWorldGridPos());
	}
	
	for (const auto& navmesh: navmeshes) {
		if (navmesh.getSubWorld() != 0) {
			continue;
		}
		for (const auto& portal: navmesh.getPortals()) {
			if (!portal.connected) {
				if (portal.local) {
					Logger::logWarning("Local Portal at " + portal.pos + " on subWorld " + toString(navmesh.getSubWorld()) + " is unlinked.");
				} else {
					const auto& base = navmesh.getNormalisedCoordinatesBase();
					const auto relPos = portal.pos - navmesh.getOffset();
					auto normalPos = Vector2f(base.inverseTransform(relPos)) * 2;
					normalPos = Vector2f(normalPos.y, -normalPos.x);
					const auto gridPos = navmesh.getWorldGridPos();
					const Vector2i gridPosOffset = Vector2i(Vector2f(std::abs(normalPos.x) >= 0.99f ? signOf(normalPos.x) : 0, std::abs(normalPos.y) >= 0.99f ? signOf(normalPos.y) : 0));
					if (occupiedGrids.find(gridPos + gridPosOffset) != occupiedGrids.end()) {
						Logger::logWarning("Scene bounds Portal with id " + toString(portal.id) + " at " + portal.pos + " on subWorld " + toString(navmesh.getSubWorld()) +  + " (between chunks at " + gridPos + " and " + (gridPos + gridPosOffset) + ") is unlinked.");
					}
				}
			}
		}
	}
}

void NavmeshSet::tryLinkNavMeshes(uint16_t idxA, uint16_t idxB)
{
	auto& a = navmeshes[idxA];
	auto& b = navmeshes[idxB];

	if (a.getSubWorld() != b.getSubWorld()) {
		// TODO: Different subworlds
		return;
	}

	const int gridDistance = (a.getWorldGridPos() - b.getWorldGridPos()).manhattanLength();
	if (gridDistance > 1) {
		// Too far
		return;
	}
	const bool localLink = gridDistance == 0;

	const auto& edgesA = a.getPortals();
	const auto& edgesB = b.getPortals();
	for (size_t edgeAIdx = 0; edgeAIdx < edgesA.size(); ++edgeAIdx) {
		const auto& edgeA = edgesA[edgeAIdx];
		if (edgeA.connected || edgeA.local != localLink) {
			continue;
		}
		
		for (size_t edgeBIdx = 0; edgeBIdx < edgesB.size(); ++edgeBIdx) {
			const auto& edgeB = edgesB[edgeBIdx];
			if (edgeB.connected || edgeB.local != localLink) {
				continue;
			}
			
			if (edgeA.canJoinWith(edgeB)) {
				// Join edges
				a.markPortalConnected(edgeAIdx);
				b.markPortalConnected(edgeBIdx);

				const auto curPos = edgeA.pos;

				// Create the portal nodes
				const uint16_t portalIdx = static_cast<uint16_t>(portalNodes.size());
				portalNodes.emplace_back(PortalNode(curPos, idxA, static_cast<uint16_t>(edgeAIdx), idxB, static_cast<uint16_t>(edgeBIdx)));
				portalNodes.emplace_back(PortalNode(curPos, idxB, static_cast<uint16_t>(edgeBIdx), idxA, static_cast<uint16_t>(edgeAIdx)));

				// Add portal node to region node
				regionNodes[idxA].portals.push_back(portalIdx);
				regionNodes[idxB].portals.push_back(portalIdx + 1);
			}
		}
	}
}

std::vector<NavmeshSet::NodeAndConn> NavmeshSet::findRegionPath(Vector2f startPos, Vector2f endPos, NodeId fromRegionId, NodeId toRegionId) const
{
	// Ensure the query is valid
	if (fromRegionId >= static_cast<int>(regionNodes.size()) || toRegionId >= static_cast<int>(regionNodes.size())) {
		// Invalid query
		return {};
	}

	// State map. Using vector for perf, trading space for CPU.
	// TODO: measure this vs unordered_map or some other hashtable, it's not impossible that it'd perform better (compact memory)
	std::vector<State> state(portalNodes.size(), State{});

	// Open set
	auto openSet = PriorityQueue<NodeId, NodeComparator>(NodeComparator(state));
	openSet.reserve(std::min(static_cast<size_t>(100), portalNodes.size()));

	// Define heuristic function
	auto h = [&] (Vector2f pos) -> float
	{
		return (pos - endPos).length();
	};

	// Initialize the query
	{
		const auto& startRegion = regionNodes[fromRegionId];
		for (const auto portalId : startRegion.portals) {
			auto& nodeState = state[portalId];
			const auto pos = portalNodes[portalId].pos;
			nodeState.cameFrom = std::numeric_limits<uint16_t>::max();
			nodeState.gScore = (pos - startPos).length();
			nodeState.fScore = h(pos);
			nodeState.inOpenSet = true;
			openSet.push(portalId);
		}
	}

	// Run A*
	while (!openSet.empty()) {
		const auto curId = openSet.top();
		const auto& curNode = portalNodes[curId];
		if (curNode.toRegion == toRegionId) {
			// A* is done! Generate result and return it
			std::vector<NodeAndConn> result;
			uint16_t portal = std::numeric_limits<uint16_t>::max();
			for (uint16_t i = curId; true;) {
				const auto& nodeData = portalNodes[i];
				result.push_back(NodeAndConn(nodeData.toRegion, portal));
				portal = nodeData.fromPortal;
				
				i = state[i].cameFrom;
				if (i == std::numeric_limits<uint16_t>::max()) {
					result.push_back(NodeAndConn(fromRegionId, portal));
					break;
				}
			}
			std::reverse(result.begin(), result.end());
			return result;
		}

		// Process current node
		state[curId].inOpenSet = false;
		state[curId].inClosedSet = true;
		openSet.pop();

		// Process neighbours
		const float gScore = state[curId].gScore;
		for (size_t i = 0; i < curNode.connections.size(); ++i) {
			const auto nodeId = curNode.connections[i].portalId;
			if (!state[nodeId].inClosedSet) {
				auto& neighState = state[nodeId];
				const float neighScore = gScore + curNode.connections[i].cost;

				// This neighbour needs updating
				if (neighScore < neighState.gScore) {
					neighState.cameFrom = curId;
					neighState.gScore = neighScore;
					neighState.fScore = neighScore + h(portalNodes[nodeId].pos);
					if (!neighState.inOpenSet) {
						neighState.inOpenSet = true;
						openSet.push(nodeId);
					} else {
						openSet.update(nodeId);
					}
				}
			}
		}
	}
	
	return {};
}

std::pair<uint16_t, uint16_t> NavmeshSet::getPortalDestination(uint16_t region, uint16_t edge) const
{
	constexpr auto maxVal = std::numeric_limits<uint16_t>::max();
	if (edge == maxVal) {
		return { maxVal, maxVal };
	}
	
	const auto& nodePortals = regionNodes.at(region).portals;
	for (const auto& portalId: nodePortals) {
		const auto& portal = portalNodes.at(portalId);
		if (portal.fromRegion == region && portal.fromPortal == edge) {
			return { portal.toRegion, portal.toPortal };
		}
	}
	
	return { maxVal, maxVal };
}

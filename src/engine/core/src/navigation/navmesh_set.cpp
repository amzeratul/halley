#include "halley/navigation/navmesh_set.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/data_structures/priority_queue.h"
#include "halley/maths/ray.h"
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
	assignNavmeshIds();
}

ConfigNode NavmeshSet::toConfigNode() const
{
	ConfigNode::MapType result;
	result["navmeshes"] = navmeshes;
	return result;
}

std::shared_ptr<NavmeshSet> NavmeshSet::loadResource(ResourceLoader& loader)
{
	auto result = std::make_shared<NavmeshSet>();
	Deserializer::fromBytes(*result, loader.getStatic()->getSpan(), SerializerOptions(SerializerOptions::maxVersion));
	return result;
}

void NavmeshSet::reload(Resource&& resource)
{
	*this = dynamic_cast<NavmeshSet&&>(resource);
}

void NavmeshSet::makeDefault()
{
}

void NavmeshSet::serialize(Serializer& s) const
{
	s << navmeshes;
}

void NavmeshSet::deserialize(Deserializer& s)
{
	s >> navmeshes;
	assignNavmeshIds();
}

void NavmeshSet::add(Navmesh navmesh)
{
	auto id = navmeshes.size();
	navmeshes.push_back(std::move(navmesh));
	navmeshes.back().setId(static_cast<uint16_t>(id));
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
	assignNavmeshIds();
}

std::optional<NavigationPath> NavmeshSet::pathfind(const NavigationQuery& origQuery, String* errorOut, float anisotropy, float nudge) const
{
	const auto [fromRegion, fromPos] = getNavMeshIdxAtWithTolerance(origQuery.from, maxStartDistanceToNavMesh, anisotropy, nudge);
	const auto [toRegion, toPos] = getNavMeshIdxAtWithTolerance(origQuery.to, maxEndDistanceToNavMesh, anisotropy, nudge);
	constexpr size_t notFound = std::numeric_limits<size_t>::max();

	if (fromRegion == notFound || toRegion == notFound) {
		// Failed
		if (errorOut) {
			if (fromRegion == notFound && toRegion == notFound) {
				*errorOut = "neither the start position " + origQuery.from + " nor the end position " + origQuery.to + " are on the navmesh";
			} else if (fromRegion == notFound) {
				*errorOut = "start position " + origQuery.from + " is not on the navmesh";
			} else {
				*errorOut = "end position " + origQuery.to + " is not on the navmesh";
			}
		}
		return {};
	}

	auto query = origQuery;
	query.from = fromPos;
	query.to = toPos;

	if (fromRegion == toRegion) {
		// Just path in that mesh
		auto path = pathfindInRegion(query, static_cast<uint16_t>(fromRegion));
		if (path) {
			postProcessPath(*path);
		}
		return path;
	} else {
		// Gotta path between regions first
		auto regionPath = findRegionPath(fromPos.pos, toPos.pos, static_cast<uint16_t>(fromRegion), static_cast<uint16_t>(toRegion));
		if (regionPath.size() <= 1) {
			// Failed
			if (errorOut) {
				*errorOut = "no path from " + fromPos + " to " + toPos;
			}
			return {};
		} else {
			auto path = extendToFullPath(query, regionPath);
			postProcessPath(path);
			return path;
		}
	}
}

std::optional<NavigationPath> NavmeshSet::pathfindInRegion(const NavigationQuery& query, uint16_t regionId) const
{
	return navmeshes[regionId].pathfind(query);
}

NavigationPath NavmeshSet::extendToFullPath(const NavigationQuery& query, const Vector<NodeAndConn>& regions) const
{
	auto result = Vector<NavigationPath::Point>();

	auto lastPosition = query.from.pos;

	for (auto i = 0; i < int(regions.size()); i++) {
		const auto& region = regions.at(i);

		const auto isLastRegion = i == int(regions.size()) - 1;
		const auto& regionNavMesh = navmeshes[region.regionNodeId];

		const auto endPos = isLastRegion ? query.to.pos : regionNavMesh.getPortals()[region.exitEdgeId].pos;
		const auto subWorld = regionNavMesh.getSubWorld();

		const auto p0 = WorldPosition(lastPosition, subWorld);
		const auto p1 = WorldPosition(endPos, subWorld);
		const auto subQuery = NavigationQuery(p0, p1, query.postProcessingType, query.quantizationType);

		auto newPath = pathfindInRegion(subQuery, region.regionNodeId);
		if (!newPath) {
			Logger::logError("Unable to find path within region from " + toString(p0) + " to " + p1, true);
			return {};
		}
		for (const auto& point: newPath->path) {
			result.emplace_back(NavigationPath::Point{ point.pos, region.regionNodeId });
		}
		lastPosition = endPos;
	}

	return NavigationPath(query, std::move(result));
}

const Navmesh* NavmeshSet::getNavMeshAt(WorldPosition pos) const
{
	for (const auto& navmesh: navmeshes) {
		if (navmesh.getSubWorld() == pos.subWorld && navmesh.containsPoint(pos.pos)) {
			return &navmesh;
		}
	}
	
	return nullptr;
}

size_t NavmeshSet::getNavMeshIdxAt(WorldPosition pos) const
{
	size_t i = 0;
	for (const auto& navmesh: navmeshes) {
		if (navmesh.getSubWorld() == pos.subWorld && navmesh.containsPoint(pos.pos)) {
			return i;
		}
		i++;
	}
	
	return std::numeric_limits<size_t>::max();
}

std::pair<size_t, WorldPosition> NavmeshSet::getNavMeshIdxAtWithTolerance(WorldPosition pos, float maxDist, float anisotropy, float nudge) const
{
	auto navMeshIdx = getNavMeshIdxAt(pos);
	if (navMeshIdx != std::numeric_limits<size_t>::max()) {
		return { navMeshIdx, pos };
	}

	// Find closest and try again
	if (auto p = getClosestPointTo(pos, maxDist, anisotropy, nudge)) {
		return { getNavMeshIdxAt(*p), *p };
	}

	return { std::numeric_limits<size_t>::max(), pos };
}

std::optional<WorldPosition> NavmeshSet::getClosestPointTo(WorldPosition pos, float maxDist, float anisotropy, float nudge, bool anySubWorld) const
{
	std::optional<WorldPosition> bestPoint;
	float bestDist = maxDist;

	for (const auto& navmesh: navmeshes) {
		if (anySubWorld || navmesh.getSubWorld() == pos.subWorld) {
			if (const auto curPoint = navmesh.getClosestPointTo(pos.pos, anisotropy, bestDist)) {
				const float dist = (*curPoint - pos.pos).length();
				if (dist < bestDist) {
					bestDist = dist;
					bestPoint = WorldPosition(*curPoint, navmesh.getSubWorld());
				}
			}
		}
	}

	if (bestPoint) {
		const auto scale = Vector2f(1.0f, anisotropy);
		const auto du = bestDist > 0.00001f ? ((bestPoint->pos - pos.pos) * scale).unit() : Vector2f(0, 1);

		const auto p1 = *bestPoint + (du / scale) * nudge;
		if (getNavMeshAt(p1)) {
			return p1;
		}

		const auto p2 = *bestPoint + (du.orthoLeft() / scale) * nudge;
		if (getNavMeshAt(p2)) {
			return p2;
		}

		const auto p3 = *bestPoint + (du.orthoRight() / scale) * nudge;
		if (getNavMeshAt(p3)) {
			return p3;
		}

		const auto p4 = *bestPoint - (du / scale) * nudge;
		if (getNavMeshAt(p4)) {
			return p4;
		}

		return bestPoint;
	}

	return {};
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

void NavmeshSet::reportUnlinkedPortals(std::function<String(Vector2i)> getChunkName) const
{
	std::set<Vector2i> occupiedGrids;
	for (const auto& navmesh: navmeshes) {
		occupiedGrids.insert(navmesh.getWorldGridPos());
	}
	
	for (const auto& navmesh: navmeshes) {
		for (const auto& portal: navmesh.getPortals()) {
			if (!portal.connected) {
				if (portal.regionLink) {
					// Local portals (always check)
					const auto gridPos = navmesh.getWorldGridPos();
					Logger::logError("\nUnlinked local Portal in \"" + getChunkName(gridPos) + "\" at " + portal.pos + ":" + toString(navmesh.getSubWorld()), true);
				} else {
					// Portals between chunks
					const auto& base = navmesh.getNormalisedCoordinatesBase();
					const auto relPos = portal.pos - navmesh.getOffset();
					auto normalPos = Vector2f(base.inverseTransform(relPos)) * 2;
					normalPos = Vector2f(normalPos.y, -normalPos.x);
					const auto gridPos = navmesh.getWorldGridPos();
					const Vector2i gridPosOffset = Vector2i(Vector2f(std::abs(normalPos.x) >= 0.99f ? signOf(normalPos.x) : 0, std::abs(normalPos.y) >= 0.99f ? signOf(normalPos.y) : 0));
					if (occupiedGrids.find(gridPos + gridPosOffset) != occupiedGrids.end()) {
						Logger::logError("\nUnlinked portal " + toString(portal.id) + " from \"" + getChunkName(gridPos) + "\" to \"" + getChunkName(gridPos + gridPosOffset)
							+ "\" at " + portal.pos + ":" + toString(navmesh.getSubWorld()) +
							+ "\n\t(portal vertices: " + String::concat(portal.vertices.span(), ", ") + ")", true);
					}
				}
			}
		}
	}
}

void NavmeshSet::setMaxDistancesToNavmesh(float startDistance, float endDistance)
{
	maxStartDistanceToNavMesh = startDistance;
	maxEndDistanceToNavMesh = endDistance;
}

void NavmeshSet::tryLinkNavMeshes(uint16_t idxA, uint16_t idxB)
{
	constexpr float epsilon = 5.0f;
	auto& a = navmeshes[idxA];
	auto& b = navmeshes[idxB];

	const int gridDistance = (a.getWorldGridPos() - b.getWorldGridPos()).manhattanLength();
	if (gridDistance > 1) {
		// Too far
		return;
	}
	const bool localLink = gridDistance == 0;
	
	const bool differentSubWorlds = a.getSubWorld() != b.getSubWorld();
	if (differentSubWorlds && !localLink) {
		return;
	}

	const bool subWorldLink = std::abs(a.getSubWorld() - b.getSubWorld()) == 1;
	const bool regionLink = localLink && !subWorldLink;

	const auto& edgesA = a.getPortals();
	const auto& edgesB = b.getPortals();
	for (size_t edgeAIdx = 0; edgeAIdx < edgesA.size(); ++edgeAIdx) {
		const auto& edgeA = edgesA[edgeAIdx];
		if (edgeA.connected || edgeA.regionLink != regionLink || edgeA.subWorldLink != subWorldLink) {
			continue;
		}
		
		for (size_t edgeBIdx = 0; edgeBIdx < edgesB.size(); ++edgeBIdx) {
			const auto& edgeB = edgesB[edgeBIdx];
			if (edgeB.connected || edgeB.regionLink != regionLink || edgeB.subWorldLink != subWorldLink) {
				continue;
			}
			
			if (edgeA.canJoinWith(edgeB, epsilon)) {
				// Join edges
				a.markPortalConnected(edgeAIdx, idxB);
				b.markPortalConnected(edgeBIdx, idxA);

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

Vector<NavmeshSet::NodeAndConn> NavmeshSet::findRegionPath(Vector2f startPos, Vector2f endPos, NodeId fromRegionId, NodeId toRegionId) const
{
	// Ensure the query is valid
	if (fromRegionId >= static_cast<int>(regionNodes.size()) || toRegionId >= static_cast<int>(regionNodes.size())) {
		// Invalid query
		return {};
	}

	// State map. Using vector for perf, trading space for CPU.
	// TODO: measure this vs unordered_map or some other hashtable, it's not impossible that it'd perform better (compact memory)
	Vector<State> state(portalNodes.size(), State{});

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
			Vector<NodeAndConn> result;
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

void NavmeshSet::postProcessPath(NavigationPath& path) const
{
	if (path.query.postProcessingType != NavigationQuery::PostProcessingType::None) {
		simplifyPath(path.path, path.query.postProcessingType);
	}
	if (path.query.quantizationType != NavigationQuery::QuantizationType::None) {
		quantizePath(path.path, path.query.quantizationType);
	}
}

void NavmeshSet::simplifyPath(Vector<NavigationPath::Point>& points, NavigationQuery::PostProcessingType type) const
{
	if (type == NavigationQuery::PostProcessingType::None || points.size() <= 2) {
		return;
	}

	// Pre-calculate node ids
	Vector<NodeId> nodeIds;
	nodeIds.resize(points.size());
	for (size_t i = 0; i < points.size(); ++i) {
		auto& navmesh = navmeshes[points[i].navmeshId];
		nodeIds[i] = navmesh.getNodeAt(points[i].pos.pos).value_or(static_cast<NodeId>(-1));
		assert(nodeIds[i] != static_cast<NodeId>(-1));
	}

	constexpr size_t maxLookAhead = 30;

	// This algorithm is run in two passes; the first one only accepts paths which are substantially shorter, the second one accepts most simplifications
	for (int pass = 0; pass < 2; ++pass) {
		float distThreshold = pass == 0 ? 0.8f : 1.25f;

		// Pre-calculate costs
		Vector<float> costs;
		costs.resize(points.size());
		for (size_t i = 0; i < points.size(); ++i) {
			if (i < points.size() - 1) {
				costs[i] = findRayCollision(points[i], points[i + 1], nodeIds[i]).second;
			} else {
				costs[i] = 0;
			}
		}

		for (size_t i = 1; i < points.size() - 1; ++i) {
			// This point can be removed if the previous point has direct line of sight to one of the points after this

			const auto p0 = points.at(i - 1);
			const auto p1 = points.at(i);
			if (p0.pos.subWorld != p1.pos.subWorld) {
				continue;
			}

			size_t bestCandidate = 0;
			float bestRatio = distThreshold;

			const auto lastCheck = std::min(points.size() - 1, i + maxLookAhead);
			float oldCost = costs[i - 1];
			for (size_t j = i + 1; j <= lastCheck; ++j) {
				const auto p2 = points.at(j);
				if (p2.pos.subWorld != p0.pos.subWorld) {
					break;
				}

				const auto col = findRayCollision(p0, p2, nodeIds[i - 1]);
				oldCost += costs[j - 1];
				
				if (col.first) {
					// Blocked, give up
					break;
				} else {
					// No collision, so this is a candidate for removal
					// (Keep searching for a better candidate)

					float costRatio = col.second / oldCost;
					if (costRatio < bestRatio) {
						bestRatio = costRatio;
						bestCandidate = j;
					}
				}
			}

			if (bestCandidate != 0) {
				points.erase(points.begin() + i, points.begin() + bestCandidate);
				nodeIds.erase(nodeIds.begin() + i, nodeIds.begin() + bestCandidate);
				costs.erase(costs.begin() + i, costs.begin() + bestCandidate);

				// i is now the next item, so decrement (so when it increments on the loop, it ends up where it started)
				// Note that this is only safe because the loop starts on 1, otherwise it could overflow and stop the loop
				--i;
			}
		}
	}
}

void NavmeshSet::quantizePath(Vector<NavigationPath::Point>& points, NavigationQuery::QuantizationType type) const
{
	if (type == NavigationQuery::QuantizationType::Quantize8Way) {
		quantizePath8Way(points, Vector2f(1, 1));
	} else if (type == NavigationQuery::QuantizationType::Quantize8WayIsometric) {
		quantizePath8Way(points, Vector2f(1, 2));
	}
}

void NavmeshSet::quantizePath8Way(Vector<NavigationPath::Point>& points, Vector2f scale) const
{
	auto makePoint = [this] (WorldPosition pos) -> std::optional<NavigationPath::Point>
	{
		auto navmeshIdx = getNavMeshIdxAt(pos);
		if (navmeshIdx == std::numeric_limits<decltype(navmeshIdx)>::max()) {
			return std::nullopt;
		}
		return NavigationPath::Point(pos, static_cast<uint16_t>(navmeshIdx));
	};

	if (points.size() < 2) {
		return;
	}

	Vector<NavigationPath::Point> result;
	result.reserve(points.size());
	result.push_back(points[0]);

	for (size_t i = 1; i < points.size(); i++) {
		const auto a = points[i - 1];
		const auto b = points[i];

		if (a.pos.subWorld != b.pos.subWorld) {
			result.push_back(b);
			continue;
		}

		// Construct a parallelogram around the path
		// One of these will be a diagonal, the other will be horizontal or vertical
		// d0 is the diagonal, d1 is the remaining (horizontal or vertical)
		const auto delta = (b.pos.pos - a.pos.pos) * scale;
		const auto d0 = std::abs(delta.x) > std::abs(delta.y) ? Vector2f(signOf(delta.x) * std::abs(delta.y), delta.y) : Vector2f(delta.x, signOf(delta.y) * std::abs(delta.x));
		const auto d1 = delta - d0;
		const auto d0s = d0 / scale;
		const auto d1s = d1 / scale;

		// If either vector is too small, then it's not worth doing it
		const float threshold = 1.0f;
		if (d0.length() >= threshold && d1.length() >= threshold) {
			// Two potential target points, c and d, are constructed
			const auto c = makePoint(a.pos + d0s);
			const auto d = makePoint(a.pos + d1s);

			// See if either path is acceptable
			if (c && isPathClear({ a, *c, b })) {
				result.push_back(*c);
			} else if (d && isPathClear({ a, *d, b })) {
				result.push_back(*d);
			} else {
				// Try a more zig-zaggy path, composed of half of one leg, then the full other leg, followed by another half leg
				const auto e = makePoint(a.pos + 0.5f * d1s);
				const auto f = makePoint(a.pos + 0.5f * d1s + d0s);
				const auto g = makePoint(a.pos + 0.5f * d0s);
				const auto h = makePoint(a.pos + 0.5f * d0s + d1s);
				if (e && f && isPathClear({ a, *e, *f, b })) {
					result.push_back(*e);
					result.push_back(*f);
				} else if (g && h && isPathClear({ a, *g, *h, b})) {
					result.push_back(*g);
					result.push_back(*h);
				}
			}
		}

		result.push_back(b);
	}

	points = std::move(result);
}

bool NavmeshSet::isPathClear(std::initializer_list<const NavigationPath::Point> points) const
{
	if (points.size() < 2) {
		return false;
	}

	for (size_t i = 1; i < points.size(); ++i) {
		if (findRayCollision(*(points.begin() + (i - 1)), *(points.begin() + i)).first) {
			return false;
		}
	}
	return true;
}

bool NavmeshSet::isPathClear(gsl::span<const NavigationPath::Point> points) const
{
	if (points.size() < 2) {
		return false;
	}

	for (size_t i = 1; i < points.size(); ++i) {
		if (findRayCollision(points[i - 1], points[i]).first) {
			return false;
		}
	}
	return true;
}

bool NavmeshSet::isPathClear(gsl::span<const WorldPosition> points) const
{
	if (points.size() < 2) {
		return false;
	}

	auto prevPoint = NavigationPath::Point(points[0], static_cast<uint16_t>(getNavMeshIdxAt(points[0])));
	auto prevNodeId = navmeshes[prevPoint.navmeshId].getNodeAt(points[0].pos);
	if (!prevNodeId) {
		return false;
	}

	for (size_t i = 1; i < points.size(); ++i) {
		auto curPoint = NavigationPath::Point(points[i], prevPoint.navmeshId);
		auto curNodeId = navmeshes[curPoint.navmeshId].getNodeAt(curPoint.pos.pos);
		if (!curNodeId) {
			curPoint.navmeshId = static_cast<uint16_t>(getNavMeshIdxAt(curPoint.pos));
			curNodeId = navmeshes[curPoint.navmeshId].getNodeAt(curPoint.pos.pos);
			if (!curNodeId) {
				return false;
			}
		}

		if (findRayCollision(prevPoint, curPoint, *prevNodeId).first) {
			return false;
		}

		prevPoint = curPoint;
		prevNodeId = curNodeId;
	}
	return true;
}

std::pair<std::optional<Vector2f>, float> NavmeshSet::findRayCollision(NavigationPath::Point from, NavigationPath::Point to) const
{
	auto& navmesh = navmeshes[from.navmeshId];
	auto nodeId = navmesh.getNodeAt(from.pos.pos);
	if (!nodeId) {
		return { std::optional(from.pos.pos), 0.0f };
	}
	return findRayCollision(from, to, *nodeId);
}

std::pair<std::optional<Vector2f>, float> NavmeshSet::findRayCollision(NavigationPath::Point from, NavigationPath::Point to, uint16_t startNodeId) const
{
	auto& navmesh = navmeshes[from.navmeshId];
	const auto delta = to.pos.pos - from.pos.pos;
	const float len = delta.length();
	const auto dir = delta / len;
	return navmesh.findRayCollision(Ray(from.pos.pos, dir), len, startNodeId, 0, this);
}

void NavmeshSet::assignNavmeshIds()
{
	for (uint16_t i = 0; i < static_cast<uint16_t>(navmeshes.size()); ++i) {
		navmeshes[i].setId(i);
	}
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

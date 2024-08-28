#include "halley/navigation/navigation_path_follower.h"

#include "halley/navigation/navmesh_set.h"
#include "halley/support/debug.h"
#include "halley/support/logger.h"
using namespace Halley;

Halley::NavigationPathFollower::NavigationPathFollower(const ConfigNode& node)
{
	if (node.hasKey("path")) {
		path = NavigationPath(node["path"]);
		needsToReEvaluatePath = true; // Path doesn't store all its info on ConfigNode, so re-query it
	}
	curPos = WorldPosition(node["curPos"]);
	nextPathIdx = node["nextPathIdx"].asInt(0);
	nextRegionIdx = node["nextRegionIdx"].asInt(0);
	params = node["params"];
}

ConfigNode NavigationPathFollower::toConfigNode() const
{
	ConfigNode::MapType result;

	if (path) {
		result["path"] = path->toConfigNode();
	}
	if (curPos != WorldPosition()) {
		result["curPos"] = curPos;
	}
	if (nextPathIdx != 0) {
		result["nextPathIdx"] = static_cast<int>(nextPathIdx);
	}
	if (nextRegionIdx != 0) {
		result["nextRegionIdx"] = static_cast<int>(nextRegionIdx);
	}
	if (params.getType() == ConfigNodeType::Map && !params.asMap().empty()) {
		result["params"] = params;
	}
	
	return result;
}

void NavigationPathFollower::setComputingPath()
{
	computingPath = true;
}

void NavigationPathFollower::setPath(std::optional<NavigationPath> p, ConfigNode params)
{
	computingPath = false;
	doSetPath(std::move(p));
	this->params = std::move(params);
	this->params.ensureType(ConfigNodeType::Map);
}

void NavigationPathFollower::doSetPath(std::optional<NavigationPath> p)
{
	path = std::move(p);
	nextPathIdx = 0;
	nextRegionIdx = 0;
	if (path) {
		navmeshSubWorld = path->query.from.subWorld;
	} else {
		navmeshSubWorld = 0;
	}
}

const std::optional<NavigationPath>& NavigationPathFollower::getPath() const
{
	return path;
}

gsl::span<const Vector2f> NavigationPathFollower::getNextPathPoints() const
{
	if (!path) {
		return {};
	}
	return path->path.span().subspan(nextPathIdx);
}

void NavigationPathFollower::update(WorldPosition curPos, const NavmeshSet& navmeshSet, float threshold)
{
	this->curPos = curPos;

	if (!path) {
		return;
	}

	if (needsToReEvaluatePath) {
		reEvaluatePath(navmeshSet);
	}
	if (!path) {
		return;
	}

	if (nextPathIdx >= path->path.size()) {
		goToNextRegion(navmeshSet);
		if (!path) {
			return;
		}
	}
	
	const auto nextPos = path->path[nextPathIdx];
	const bool arrivedAtNextNode = (nextPos - curPos.pos).squaredLength() < threshold * threshold;
	
	if (arrivedAtNextNode) {
		nextPathIdx++;
		if (nextPathIdx >= path->path.size()) {
			goToNextRegion(navmeshSet);
		}
	}
}

void NavigationPathFollower::goToNextRegion(const NavmeshSet& navmeshSet)
{
	Expects(path.has_value());

	if (nextRegionIdx < path->regions.size()) {
		// Last position on the last path
		const auto startPos = path->path.empty() ? path->query.from.pos : path->path.back();
		
		// Get next region and figure where in it we want to end up at
		const bool isLastRegion = nextRegionIdx == path->regions.size() - 1;
		const auto& region = path->regions[nextRegionIdx];
		const auto& regionNavMesh = navmeshSet.getNavmeshes()[region.regionNodeId];

		Vector2f endPos;
		if (isLastRegion) {
			// Simply target the end of query
			endPos = path->query.to.pos;
		} else {
			// First find where approximately we'll end on the next region
			const auto& portal = regionNavMesh.getPortals().at(region.exitEdgeId);
			const auto& secondRegion = path->regions[nextRegionIdx + 1];
			const auto& secondRegionNavMesh = navmeshSet.getNavmeshes()[secondRegion.regionNodeId];
			const auto secondEndPos = nextRegionIdx + 1 == path->regions.size() - 1 ? path->query.to.pos : secondRegionNavMesh.getPortals()[secondRegion.exitEdgeId].pos;

			// Aim for the closest point on the portal
			endPos = portal.getClosestPoint(secondEndPos);
		}

		// Run query
		const int subWorld = regionNavMesh.getSubWorld();
		const auto query = NavigationQuery(WorldPosition(startPos, subWorld), WorldPosition(endPos, subWorld), path->query.postProcessingType, path->query.quantizationType);
		const auto newPath = navmeshSet.pathfindInRegion(query, region.regionNodeId);

		if (newPath && !newPath->path.empty()) {
			// Set new path
			path->path = std::move(newPath->path);
			nextPathIdx = 0;
			nextRegionIdx++;
			navmeshSubWorld = subWorld;
		} else {
			nextSubPath();
		}
	} else {
		// No more regions
		nextSubPath();
	}
}

void NavigationPathFollower::nextSubPath()
{
	assert(path.has_value());

	if (path->followUpPaths.empty()) {
		doSetPath({});
	} else {
		auto followUpPaths = std::move(path->followUpPaths);
		auto newPath = std::move(followUpPaths.front());
		followUpPaths.erase(followUpPaths.begin());
		newPath.followUpPaths = std::move(followUpPaths);
		doSetPath(std::move(newPath));
	}
}

void NavigationPathFollower::reEvaluatePath(const NavmeshSet& navmeshSet)
{
	needsToReEvaluatePath = false;
	auto query = path->query;
	query.from = curPos;
	doSetPath(navmeshSet.pathfind(query));
}

Vector2f NavigationPathFollower::getNextPosition() const
{
	return path->path.size() > nextPathIdx ? path->path[nextPathIdx] : curPos.pos;
}

size_t NavigationPathFollower::getNextPathIdx() const
{
	return nextPathIdx;
}

uint16_t NavigationPathFollower::getCurrentRegionId() const
{
	if (!path) {
		return std::numeric_limits<uint16_t>::max();
	}
	if (path->regions.empty()) {
		return std::numeric_limits<uint16_t>::max();
	}
	if (path->regions.size() == 1) {
		return path->regions[0].regionNodeId;
	}
	return path->regions[nextRegionIdx - 1].regionNodeId;
}

bool NavigationPathFollower::isDone() const
{
	return !path;
}

void NavigationPathFollower::setNavmeshSubWorld(int value)
{
	navmeshSubWorld = value;
}

int NavigationPathFollower::getNavmeshSubWorld() const
{
	return navmeshSubWorld;
}

const ConfigNode& NavigationPathFollower::getParams() const
{
	return params;
}

ConfigNode& NavigationPathFollower::getParams()
{
	return params;
}

ConfigNode ConfigNodeSerializer<NavigationPathFollower>::serialize(const NavigationPathFollower& follower, const EntitySerializationContext& context)
{
	return follower.toConfigNode();
}

NavigationPathFollower ConfigNodeSerializer<NavigationPathFollower>::deserialize(const EntitySerializationContext& context,	const ConfigNode& node)
{
	return NavigationPathFollower(node);
}

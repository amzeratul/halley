#include "halley/navigation/navigation_path_follower.h"

#include "halley/navigation/navmesh_set.h"
#include "halley/support/logger.h"
using namespace Halley;

Halley::NavigationPathFollower::NavigationPathFollower(const ConfigNode& node)
{
	if (node.hasKey("path")) {
		path = NavigationPath(node["path"]);
		needsToReEvaluatePath = true; // Path doesn't store all its info on ConfigNode, so re-query it
	}
	curPos = node["curPos"].asVector2f(Vector2f());
	curSubWorld = node["curSubWorld"].asInt(0);
	nextPathIdx = node["nextPathIdx"].asInt(0);
	nextRegionIdx = node["nextRegionIdx"].asInt(0);
}

ConfigNode NavigationPathFollower::toConfigNode() const
{
	ConfigNode::MapType result;

	if (path) {
		result["path"] = path->toConfigNode();
	}
	if (curPos != Vector2f()) {
		result["curPos"] = curPos;
	}
	if (curSubWorld != 0) {
		result["curSubWorld"] = curSubWorld;
	}
	if (nextPathIdx != 0) {
		result["nextPathIdx"] = static_cast<int>(nextPathIdx);
	}
	if (nextRegionIdx != 0) {
		result["nextRegionIdx"] = static_cast<int>(nextRegionIdx);
	}
	
	return result;
}

void NavigationPathFollower::setPath(std::optional<NavigationPath> p)
{
	path = std::move(p);
	nextPathIdx = 0;
	nextRegionIdx = 0;
}

const std::optional<NavigationPath>& NavigationPathFollower::getPath() const
{
	return path;
}

void NavigationPathFollower::update(Vector2f curPos, int curSubWorld, const NavmeshSet& navmeshSet, float threshold)
{
	this->curPos = curPos;
	this->curSubWorld = curSubWorld;

	if (!path) {
		return;
	}

	if (needsToReEvaluatePath) {
		reEvaluatePath(navmeshSet);
	}

	if (nextPathIdx >= path->path.size()) {
		goToNextRegion(navmeshSet);
		if (!path) {
			return;
		}
	}

	const auto nextPos = path->path[nextPathIdx];
	const bool arrivedAtNextNode = (nextPos - curPos).length() < threshold;
	
	if (arrivedAtNextNode) {
		nextPathIdx++;
		if (nextPathIdx >= path->path.size()) {
			goToNextRegion(navmeshSet);
		}
	}
}

void NavigationPathFollower::goToNextRegion(const NavmeshSet& navmeshSet)
{
	if (nextRegionIdx < path->regions.size()) {
		// Last position on the last path
		const auto startPos = path->path.empty() ? path->query.from : path->path.back();
		
		// Get next region and figure where in it we want to end up at
		const bool isLastRegion = nextRegionIdx == path->regions.size() - 1;
		const auto& region = path->regions[nextRegionIdx];
		const auto& regionNavMesh = navmeshSet.getNavmeshes()[region.regionNodeId];
		const auto endPos = isLastRegion ? path->query.to : regionNavMesh.getPortals()[region.exitEdgeId].pos;

		// Run the new query for just this region
		const int subWorld = regionNavMesh.getSubWorld();

		std::optional<NavigationPath> newPath = {};

		if (isLastRegion || path->query.postProcessingType == NavigationQuery::PostProcessingType::None) {
			const auto query = NavigationQuery(startPos, subWorld, endPos, subWorld, path->query.postProcessingType);
			newPath = navmeshSet.pathfindInRegion(query, region.regionNodeId);
		}else {
			// Pathfind between regions
			const auto& secondRegion = path->regions[nextRegionIdx + 1];
			const auto& secondRegionNavMesh = navmeshSet.getNavmeshes()[secondRegion.regionNodeId];
			const auto secondEndPos = nextRegionIdx + 1 == path->regions.size() - 1 ? path->query.to : secondRegionNavMesh.getPortals()[secondRegion.exitEdgeId].pos;

			const auto queryStart = NavigationQuery(startPos, subWorld, endPos, subWorld, NavigationQuery::PostProcessingType::None);
			const auto queryEnd = NavigationQuery(endPos, subWorld, secondEndPos, secondRegionNavMesh.getSubWorld(), NavigationQuery::PostProcessingType::None);
			newPath = navmeshSet.pathfindBetweenRegions(queryStart, queryEnd, region.regionNodeId, secondRegion.regionNodeId, path->query.postProcessingType);
		}

		if (newPath && !newPath->path.empty()) {
			// Set new path
			path->path = std::move(newPath->path);
			nextPathIdx = 0;
			nextRegionIdx++;
		} else {
			Logger::logWarning("Failed to find path within region.");
			setPath({});
		}
	} else {
		// No more regions
		setPath({});
	}
}

void NavigationPathFollower::reEvaluatePath(const NavmeshSet& navmeshSet)
{
	needsToReEvaluatePath = false;
	auto query = path->query;
	query.from = curPos;
	query.fromSubWorld = curSubWorld;
	setPath(navmeshSet.pathfind(query));
}

Vector2f NavigationPathFollower::getNextPosition() const
{
	return path->path.size() > nextPathIdx ? path->path[nextPathIdx] : curPos;
}

size_t NavigationPathFollower::getNextPathIdx() const
{
	return nextPathIdx;
}

bool NavigationPathFollower::isDone() const
{
	return !path;
}

ConfigNode ConfigNodeSerializer<NavigationPathFollower>::serialize(const NavigationPathFollower& follower, const ConfigNodeSerializationContext& context)
{
	return follower.toConfigNode();
}

NavigationPathFollower ConfigNodeSerializer<NavigationPathFollower>::deserialize(const ConfigNodeSerializationContext& context,	const ConfigNode& node)
{
	return NavigationPathFollower(node);
}

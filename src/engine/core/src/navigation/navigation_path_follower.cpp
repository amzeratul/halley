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
}

const std::optional<NavigationPath>& NavigationPathFollower::getPath() const
{
	return path;
}

gsl::span<const NavigationPath::Point> NavigationPathFollower::getNextPathPoints() const
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
		nextSubPath();
		if (!path) {
			return;
		}
	}
	
	const auto nextPos = path->path[nextPathIdx];
	const bool arrivedAtNextNode = (nextPos.pos.pos - curPos.pos).squaredLength() < threshold * threshold;
	
	if (arrivedAtNextNode) {
		nextPathIdx++;
		if (nextPathIdx >= path->path.size()) {
			nextSubPath();
		}
	}
}

void NavigationPathFollower::nextSubPath()
{
	assert(path.has_value());

	doSetPath({});
}

void NavigationPathFollower::reEvaluatePath(const NavmeshSet& navmeshSet)
{
	needsToReEvaluatePath = false;
	auto query = path->query;
	query.from = curPos;
	doSetPath(navmeshSet.pathfind(query));
}

WorldPosition NavigationPathFollower::getNextPosition() const
{
	return getPointAtIdx(nextPathIdx);
}

WorldPosition NavigationPathFollower::getPointAtIdx(size_t idx) const
{
	if (!path || path->path.empty()) {
		return curPos;
	}
	if (nextPathIdx < path->path.size()) {
		return path->path[idx].pos;
	}
	return path->path.back().pos;
}

size_t NavigationPathFollower::getNextPathIdx() const
{
	return nextPathIdx;
}

bool NavigationPathFollower::isFollowingPath() const
{
	return !!path;
}

bool NavigationPathFollower::isDone() const
{
	return !path;
}

void NavigationPathFollower::detachFromNavmesh()
{
	if (path) {
		for (auto& p: path->path) {
			p.navmeshId = std::numeric_limits<uint16_t>::max();
			p.pos.subWorld = -1;
		}
	}
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

#include "halley/navigation/navigation_path_follower.h"

#include "halley/navigation/navmesh_set.h"
#include "halley/support/debug.h"
#include "halley/support/logger.h"
using namespace Halley;

NavigationPathParams::NavigationPathParams(const ConfigNode& node)
{
	run = node["run"].asBool(false);
	backwards = node["backwards"].asBool(false);
	manualPath = node["manualPath"].asBool(false);
	speed = node["speed"].asFloat(1.0f);
	if (node.hasKey("faceAfter") && node["faceAfter"].getType() == ConfigNodeType::Int) {
		faceAfter = node["faceAfter"].asInt();
	}
}

ConfigNode NavigationPathParams::toConfigNode() const
{
	ConfigNode result;
	if (run) {
		result["run"] = true;
	}
	if (backwards) {
		result["backwards"] = true;
	}
	if (manualPath) {
		result["manualPath"] = true;
	}
	if (!floatEquals(speed, 1.0f)) {
		result["speed"] = speed;
	}
	if (faceAfter) {
		result["faceAfter"] = *faceAfter;
	}
	return result;
}

Halley::NavigationPathFollower::NavigationPathFollower(const ConfigNode& node)
{
	if (node.hasKey("path")) {
		path = NavigationPath(node["path"]);
		needsToReEvaluatePath = true; // Path doesn't store all its info on ConfigNode, so re-query it
	}
	curPos = WorldPosition(node["curPos"]);
	nextPathIdx = node["nextPathIdx"].asInt(0);
	params = NavigationPathParams(node["params"]);
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

	auto p = params.toConfigNode();
	if (p.getType() == ConfigNodeType::Map && !p.asMap().empty()) {
		result["params"] = std::move(p);
	}
	
	return result;
}

void NavigationPathFollower::feedToHasher(Hash::Hasher& hasher) const
{
	ConfigNodeHelper<decltype(path)>::hash(path, hasher);
	ConfigNodeHelper<decltype(curPos)>::hash(curPos, hasher);
	ConfigNodeHelper<decltype(nextPathIdx)>::hash(nextPathIdx, hasher);
	
	hasher.feed(params.run);
	hasher.feed(params.backwards);
	hasher.feed(params.manualPath);
	hasher.feed(params.speed);
	ConfigNodeHelper<decltype(params.faceAfter)>::hash(params.faceAfter, hasher);
}

void NavigationPathFollower::setComputingPath()
{
	computingPath = true;
}

void NavigationPathFollower::clear()
{
	computingPath = false;
	doSetPath({});
	params = {};
}

void NavigationPathFollower::setPath(std::optional<NavigationPath> p, NavigationPathParams params)
{
	computingPath = false;
	doSetPath(std::move(p));
	this->params = std::move(params);
}

void NavigationPathFollower::doSetPath(std::optional<NavigationPath> p)
{
	path = std::move(p);
	nextPathIdx = 0;
	distPathCache = {};
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
	auto span = path->path.span();
	return nextPathIdx <= span.size() ? span.subspan(nextPathIdx) : gsl::span<const NavigationPath::Point>();
}

void NavigationPathFollower::update(WorldPosition curPos, const NavmeshSet& navmeshSet, float minThreshold, float maxThreshold)
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

	const bool isLastPoint = nextPathIdx + 1 == path->path.size();
	const float threshold = isLastPoint ? minThreshold : maxThreshold;
	const auto nextPos = path->path[nextPathIdx];
	const bool arrivedAtNextNode = (nextPos.pos.pos - curPos.pos).squaredLength() < threshold * threshold;

	if (arrivedAtNextNode) {
		nextPathIdx++;
		distPathCache = {};
		if (nextPathIdx >= path->path.size()) {
			nextSubPath();
		}
	}
}

void NavigationPathFollower::nextSubPath()
{
	HalleyAssertDev(path.has_value());

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

float NavigationPathFollower::getDistanceLeft(float anisotropy) const
{
	if (!path) {
		return 0;
	}
	if (nextPathIdx >= path->path.size()) {
		return 0;
	}

	const auto nextPos = path->path[nextPathIdx];
	const float distToNext = ((curPos.pos - nextPos.pos.pos) * Vector2f(1, anisotropy)).length();
	if (!distPathCache) {
		distPathCache = path->getLength(anisotropy, nextPathIdx);
	}
	return distToNext + *distPathCache;
}

const NavigationPathParams& NavigationPathFollower::getParams() const
{
	return params;
}

NavigationPathParams& NavigationPathFollower::getParams()
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

void ConfigNodeSerializer<NavigationPathFollower>::hash(const NavigationPathFollower& follower, Hash::Hasher& hasher)
{
	follower.feedToHasher(hasher);
}

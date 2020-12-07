/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/


#include "halley/maths/polygon.h"
#include <limits>

#include "halley/file_formats/config_file.h"
#include "halley/maths/ray.h"
#include "halley/maths/circle.h"
#include "halley/maths/line.h"
#include "halley/utils/algorithm.h"
using namespace Halley;


///////////////
// Constructor
Polygon::Polygon()
{
}

Polygon::Polygon(VertexList vertices)
	: vertices(std::move(vertices))
{
	realize();
}

Polygon::Polygon(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Sequence) {
		vertices.reserve(node.asSequence().size());
		for (auto& n: node.asSequence()) {
			vertices.push_back(n.asVector2f());
		}
		realize();
	}
}

void Polygon::realize()
{
	checkConvex();
	aabb = Rect4f::getSpanningRect(vertices);
	circle = Circle::getSpanningCircle(vertices);

	valid = vertices.size() >= 3;
}

void Polygon::checkConvex()
{
	size_t n = vertices.size();
	if (n < 3) {
		convex = true;
		clockwise = true;
	}
	
	size_t left = 0;
	size_t right = 0;
	float area2 = 0;

	float epsilon = 0.000001f;
	for (size_t i = 0; i < n; ++i) {
		auto a = vertices[i];
		auto b = vertices[(i + 1) % n];
		auto c = vertices[(i + 2) % n];
		const float cross = (b - a).normalized().cross((c - b).normalized());
		if (cross > epsilon) {
			right++;
		} else if (cross < -epsilon) {
			left++;
		}

		area2 += (a.x - b.x) * (a.y + b.y);
	}

	// Is convex if all turns were right turns, or all turns were left turns
	convex = right == 0 || left == 0;

	// Clockwise if the area is positive
	clockwise = area2 > 0;
}

void Polygon::simplify(float epsilon)
{
	if (vertices.size() <= 3) {
		return;
	}

	bool changed = false;
	
	size_t n = vertices.size();
	Vector2f prev = vertices.back();
	for (size_t i = 0; i < n && n > 3;) {
		const Vector2f cur = vertices[i];
		const Vector2f next = vertices[(i + 1) % n];

		if (cur.epsilonEquals(prev, epsilon) || LineSegment(prev, next).contains(cur, epsilon)) {
			vertices.erase(vertices.begin() + i);
			--n;
			changed = true;
		} else {
			prev = cur;
			++i;
		}
	}

	if (changed) {
		realize();
	}
}

bool Polygon::isPointInside(Vector2f point) const
{
	// Fast fail
	if (!circle.contains(point)) {
		return false;
	}
	if (!aabb.contains(point)) {
		return false;
	}

	// Check
	if (convex) {
		return isPointInsideConvex(point);
	} else {
		return isPointInsideConcave(point);
	}
}

bool Polygon::isPointOnEdge(Vector2f point, float epsilon) const
{
	const size_t n = vertices.size();
	for (size_t i = 0; i < n; ++i) {
		const auto edge = LineSegment(vertices[i], vertices[(i + 1) % n]);
		auto p2 = edge.getClosestPoint(point);
		if ((p2 - point).squaredLength() < epsilon * epsilon) {
			return true;
		}
	}
	return false;
}

bool Polygon::isPointInsideConvex(Vector2f point) const
{
	// Do cross product with all the segments
	const size_t len = vertices.size();
	for (size_t i = 0; i < len; i++) {
		const auto a = point - vertices[i];
		const auto b = vertices[(i+1) % len] - vertices[i];
		if (a.cross(b) > 0) {
			return false;
		}
	}

	// Nothing failed, so it's inside
	return true;
}

bool Polygon::isPointInsideConcave(Vector2f point) const
{
	size_t nLeft = 0;
	size_t nRight = 0;
	const size_t len = vertices.size();

	// For each segment that overlaps this point vertically, classify it as "left" or "right"
	for (size_t i = 0; i < len; i++) {
		const auto a = vertices[i];
		const auto b = vertices[(i+1) % len];
		auto r = Range<float>(a.y, b.y);
		if (r.contains(point.y)) {
			if (a.x < point.x && b.x < point.x) {
				nLeft++;
			} else if (a.x > point.x && b.x > point.x) {
				nRight++;
			} else {
				const float t = (point.y - a.y) / (b.y - a.y);
				const float refX = lerp(a.x, b.x, t);
				if (refX < point.x) {
					nLeft++;
				} else {
					nRight++;
				}
			}
		}
	}

	return (nLeft % 2) == 1 && (nRight % 2) == 1;
}

Vector2f average(Vector<Vector2f>& v)
{
	Vector2f result;
	size_t len = v.size();
	for (size_t i=0; i<len; i++) {
		result += v[i];
	}
	result /= float(len);
	return result;
}

bool Polygon::collide(const Polygon &param, Vector2f *translation, Vector2f *collisionPoint) const
{	
	if (convex && param.convex) {
		return collideConvex(param, translation, collisionPoint);
	} else {
		throw Exception("Cannot check collision between non-convex polygons", HalleyExceptions::Utils);
	}
}

bool Polygon::collideConvex(const Polygon& param, Vector2f* translation, Vector2f* collisionPoint) const
{
	// Using the separating axis theorem here
	// Check if they are within overlap range
	const float maxDist = circle.getRadius() + param.circle.getRadius();
	if ((circle.getCentre() - param.circle.getCentre()).squaredLength() >= maxDist * maxDist) {
		return false;
	}

	// AABB test
	//if (!aabb.overlaps(param.aabb, param.getOrigin()-getOrigin())) return false;

	// Prepare
	float minDist = -999999.0f;
	Vector2f bestAxis;
	bool hasBestAxis = false;
	float bmin1=0, bmax1=0, bmin2=0, bmax2=0;

	// For each edge
	size_t len1 = vertices.size();
	size_t len2 = param.vertices.size();
	for (size_t i=0; i<len1+len2; i++) {
		// Find the orthonormal axis
		Vector2f axis;
		if (i < len1) axis = (vertices[(i+1)%len1] - vertices[i]).orthoLeft().unit();
		else axis = (param.vertices[(i-len1+1)%len2] - param.vertices[i-len1]).orthoLeft().unit();

		// Project both polygons there
		const auto range1 = project(axis);
		const auto range2 = param.project(axis);

		// Find the distance between the projections
		const float dist = range1.start < range2.start ? range2.start - range1.end : range1.start - range2.end;
		if (dist >= 0) {
			// This axis separates them
			return false;
		} else {
			if (translation && dist > minDist) {
				bestAxis = axis;
				hasBestAxis = true;
				minDist = dist;
				bmin1 = range1.start;
				bmin2 = range2.start;
				bmax1 = range1.end;
				bmax2 = range2.end;
			}
		}
	}

	// Gather additional data based on best axis
	if (hasBestAxis) {
		// Find all vertices possibly involved in the collision
		float dist;
		int sign;
		Vector<Vector2f> v1,v2;
		if (bmin1 < bmin2) {
			dist = bmin2 - bmax1;
			sign = 1;
			if (collisionPoint) {
				unproject(bestAxis,bmax1,v1);
				param.unproject(bestAxis,bmin2,v2);
			}
		}
		else {
			dist = bmin1 - bmax2;
			sign = -1;
			if (collisionPoint) {
				unproject(bestAxis,bmin1,v1);
				param.unproject(bestAxis,bmax2,v2);
			}
		}

		// Find the collision point
		if (collisionPoint) {
			Vector2f colPoint = (circle.getCentre() + param.circle.getCentre()) / 2.0f;
			if (v1.size() == 1) {
				colPoint = v1[0];
			} else if (v2.size() == 1) {
				colPoint = v2[0];
			} else if (!v1.empty()) {
				colPoint = average(v1); //v1[0];
			} else if (!v2.empty()) {
				colPoint = average(v2); //v2[0];
			}
			*collisionPoint = colPoint;
		}

		// Find the translation vector
		*translation = bestAxis*(dist*sign);
	}

	// Done
	return true;
}

Vector2f Polygon::getClosestPoint(Vector2f rawPoint, float anisotropy) const
{
	Expects(!vertices.empty());

	const auto scale = Vector2f(1.0f, 1.0f / anisotropy);
	const auto point = rawPoint * scale;
	
	Vector2f bestPoint = vertices[0];
	float closestDistance2 = std::numeric_limits<float>::infinity();
	
	const size_t n = vertices.size();
	for (size_t i = 0; i < n; ++i) {
		const Vector2f p = LineSegment(vertices[i] * scale, vertices[(i + 1) % n] * scale).getClosestPoint(point);

		const float dist2 = (point - p).squaredLength();
		if (dist2 < closestDistance2) {
			closestDistance2 = dist2;
			bestPoint = p;
		}
	}

	return bestPoint * Vector2f(1.0f, anisotropy);
}

Polygon::SATClassification Polygon::classify(const Polygon& other) const
{
	Expects(convex);
	Expects(other.convex);

	// If bounding circles don't overlap, then the polygons definitely don't overlap
	const float maxDist = circle.getRadius() + other.circle.getRadius();
	if ((circle.getCentre() - other.circle.getCentre()).squaredLength() >= maxDist * maxDist) {
		return SATClassification::Separate;
	}
	
	bool contains = true;
	bool isContainedBy = true;

	// For each polygon
	for (size_t j = 0; j < 2; ++j) {
		const auto& vs = j == 0 ? vertices : other.vertices;
		const size_t n = vs.size();
		// For each edge
		for (size_t i = 0; i < n; i++) {
			// Find the orthonormal axis
			const Vector2f axis = (vs[(i + 1) % n] - vs[i]).orthoLeft().unit();

			// Project both polygons there
			const auto myRange = project(axis);
			const auto otherRange = other.project(axis);

			if (myRange.overlaps(otherRange)) {
				if (!myRange.contains(otherRange)) {
					contains = false;
				}
				if (!otherRange.contains(myRange)) {
					isContainedBy = false;
				}
			} else {
				// This axis separates them
				return SATClassification::Separate;
			}
		}
	}

	if (contains) {
		// This contains the other entirely
		return SATClassification::Contains;
	} else if (isContainedBy) {
		// This is contained entirely by the other
		return SATClassification::IsContainedBy;
	}
	
	// They overlap without fully containing each other
	return SATClassification::Overlap;
}

Range<float> Polygon::project(Vector2f axis) const
{
	float min = std::numeric_limits<float>::infinity();
	float max = -std::numeric_limits<float>::infinity();

	const size_t len = vertices.size();
	for (size_t i = 0; i < len; i++) {
		const float dot = axis.dot(vertices[i]);
		min = std::min(min, dot);
		max = std::max(max, dot);
	}

	return Range<float>(min, max);
}

void Polygon::unproject(const Vector2f &axis,const float point,Vector<Vector2f> &ver) const
{
	const size_t len = vertices.size();
	for (size_t i = 0; i < len; i++) {
		const float dot = axis.dot(vertices[i]);
		if (dot == point) {
			ver.push_back(vertices[i]);
		}
	}
}

LineSegment Polygon::getEdge(size_t idx) const
{
	return LineSegment(vertices[idx], vertices[(idx + 1) % vertices.size()]);
}

std::optional<size_t> Polygon::findEdge(const LineSegment& edge, float epsilon) const
{
	const size_t n = vertices.size();
	for (size_t i = 0; i < n; ++i) {
		if (getEdge(i).epsilonEquals(edge, epsilon)) {
			return i;
		}
	}
	return {};
}

void Polygon::rotate(Angle<float> angle)
{
	for (auto& v: vertices) {
		v = v.rotate(angle);
	}
	realize();
}

void Polygon::rotateAndScale(Angle<float> angle, Vector2f scale)
{
	for (auto& v: vertices) {
		v = (v * scale).rotate(angle);
	}
	realize();
}

void Polygon::scale(Vector2f scale)
{
	for (auto& v: vertices) {
		v *= scale;
	}
	realize();
}

void Polygon::expand(float amount, float truncateThreshold)
{
	Expects(convex);
	
	const float windingDir = clockwise ? -1.0f : 1.0f;

	const size_t numVertices = vertices.size();
	VertexList newVertices;
	
	for (size_t i = 0; i < numVertices; ++i) {
		const Vector2f prev = vertices[(i + numVertices - 1) % numVertices];
		const Vector2f cur = vertices[i];
		const Vector2f next = vertices[(i + 1) % numVertices];

		const Vector2f m = (cur - prev).normalized();
		const Vector2f n = (next - cur).normalized();
		const Vector2f u = m.orthoLeft() * windingDir;
		const Vector2f v = n.orthoLeft() * windingDir;

		auto lineA = Line(cur + u * amount, m);
		auto lineB = Line(cur + v * amount, n);

		if (std::abs(u.dot(v)) >= 0.999999f) {
			// Parallel
			newVertices.push_back(cur + u * amount);
		} else {
			const Vector2f newVertexPos = lineA.intersection(lineB).value();
			const Vector2f w = (u + v).normalized();
			const Vector2f idealVert = cur + w * amount;
			
			if ((newVertexPos - idealVert).length() > truncateThreshold) {
				// Truncate this vertex
				auto lineC = Line(idealVert, w.orthoRight());
				newVertices.push_back(lineA.intersection(lineC).value());
				newVertices.push_back(lineB.intersection(lineC).value());
			} else {
				// OK to go as a single vertex
				newVertices.push_back(newVertexPos);
			}
		}
	}

	setVertices(std::move(newVertices));
}

void Polygon::invertWinding()
{
	std::reverse(vertices.begin(), vertices.end());
	realize();
}

Polygon Polygon::convolution(const Polygon& other) const
{
	Expects(clockwise == other.clockwise);
	
	VertexList result;

	// For each vertex, find the "other" polygon vertex that matches the direction we're moving, and use it
	// Insert any "skipped" vertices
	const auto findVertex = [&] (const Vector2f dir) -> size_t
	{
		const auto& vs = other.vertices;
		const auto n = dir.orthoRight();
		size_t best = 0;
		float bestVal = -std::numeric_limits<float>::infinity();
		for (size_t i = 0; i < vs.size(); ++i) {
			const float val = vs[i].dot(n);
			if (val > bestVal) {
				bestVal = val;
				best = i;
			}
		}
		return best;
	};

	const auto n = vertices.size();
	const auto nConv = other.vertices.size();

	const Vector2f lastDir = (vertices.front() - vertices.back()).normalized();
	size_t lastVertex = findVertex(lastDir);
	const auto insertVertex = [&] (Vector2f base, size_t idx)
	{
		for (size_t i = lastVertex; ; i = (i + 1) % nConv) {
			result.push_back(base + other.vertices[i]);
			if (i == idx) {
				lastVertex = idx;
				break;
			}
		}
	};
	
	for (size_t i = 0; i < n; ++i) {
		const auto cur = vertices[i];
		const auto next = vertices[(i + 1) % n];
		const auto dir = (next - cur).normalized();

		insertVertex(cur, findVertex(dir));
	}

	return Polygon(result);
}

std::vector<Polygon> Polygon::splitIntoConvex() const
{
	std::vector<Polygon> result;
	splitIntoConvex(result);
	return result;
}

void Polygon::splitIntoConvex(std::vector<Polygon>& output) const
{
	Expects(isValid());
	
	if (isConvex()) {
		output.push_back(*this);
		return;
	}

	struct Score {
		int divs = 0;
		float dist = std::numeric_limits<float>::infinity();

		Score() = default;
		Score(int divs, float dist) : divs(divs), dist(dist) {}

		bool operator>(const Score& other) const
		{
			if (divs != other.divs) {
				return divs > other.divs;
			}
			return dist < other.dist;
		}
	};

	// Not convex. Here's the algorithm:
	// 1. Find all vertices whose internal angle >180
	// 2. For each of them, find all the angles it can split with (because it wouldn't overlap any of the existing edges)
	// 3. Pick the best edge
	//  3.1. Edges that split another >180 angle have priority (fewer total polygons)
	//  3.2. Smaller edges have priority
	// 4. Split on that edge, recurse on the two new polygons

	// Collect all concave angles
	const float angleSign = clockwise ? 1.0f : -1.0f;
	std::vector<size_t> concaveVertices;
	const size_t n = vertices.size();
	std::vector<char> isConcave(n, false);
	for (size_t i = 0; i < n; ++i) {
		const auto a = vertices[(i + n - 1) % n];
		const auto b = vertices[i];
		const auto c = vertices[(i + 1) % n];

		const float angle = (c - b).cross(b - a) * angleSign;

		if (angle > 0) {
			concaveVertices.emplace_back(i);
			isConcave[i] = true;
		}
	}

	Score bestScore;
	std::pair<size_t, size_t> bestSplit = {0, 0};

	// Find the best edge
	for (size_t i: concaveVertices) {
		const Vector2f a = vertices[i];
		const Vector2f prevA = vertices[(i + n - 1) % n];
		const Vector2f nextA = vertices[(i + 1) % n];
		
		for (size_t j = 0; j < n; ++j) {
			// Cannot be the same or adjacent
			if (std::abs(static_cast<int>(i) - static_cast<int>(j)) <= 1 || (std::min(i, j) == 0 && std::max(i, j) == n -1)) {
				continue;
			}

			const bool isDoubleConcave = isConcave[j];
			if (isDoubleConcave && j < i) {
				// Already checked
				continue;
			}
			const Vector2f b = vertices[j];

			// Potential splitting edge between a and b, check for cone line of sight
			const Vector2f prevB = vertices[(j + n - 1) % n];
			const Vector2f nextB = vertices[(j + 1) % n];
			const int insideResultBPov = isInsideAngle(prevB, b, nextB, a, clockwise);
			if (insideResultBPov == 0) {
				// Not in line of sight
				continue;
			}

			// Compute the score
			const int insideResultAPov = isInsideAngle(prevA, a, nextA, b, clockwise);
			const float dist = (a - b).length();
			const int divs = (insideResultAPov == 2 ? 1 : 0) + (isDoubleConcave && insideResultBPov == 2 ? 1 : 0);
			const auto score = Score(divs, dist);
			if (bestScore > score) {
				// This isn't better than the previous, abort
				continue;
			}

			// If we got here, we want to take this edge, provided it's valid.
			// Last step is to check it against every edge to make sure it doesn't overlap them
			if (overlapsEdge(LineSegment(a, b))) {
				continue;
			}

			// Valid splitting edge
			bestScore = score;
			bestSplit = { i, j };
		}
	}

	if (bestSplit.first == bestSplit.second) {
		int a = 0;
	}

	if (bestSplit.first > bestSplit.second) {
		std::swap(bestSplit.first, bestSplit.second);
	}

	assert(bestSplit.first != bestSplit.second);
	assert(bestSplit.second > bestSplit.first + 1);
	assert(bestSplit.first != 0 || bestSplit.second != vertices.size() - 1);

	// Split and recurse
	auto [poly0, poly1] = doSplit(bestSplit.first, bestSplit.second, {});
	poly0.splitIntoConvex(output);
	poly1.splitIntoConvex(output);
}

std::pair<Polygon, Polygon> Polygon::doSplit(size_t v0, size_t v1, gsl::span<const Vector2f> insertVertices) const
{
	if (v0 > v1) {
		std::vector<Vector2f> inserts(insertVertices.begin(), insertVertices.end());
		std::reverse(inserts.begin(), inserts.end());
		return doSplit(v1, v0, inserts);
	}

	Expects(!insertVertices.empty() || v1 - v0 > 1);

	//auto vs = vertices;
	//std::rotate(vs.begin(), vs.begin() + v0, vs.end());
	//auto vs0 = VertexList(vs.begin(), vs.begin() + (v1 - v0 + 1));
	//auto vs1 = VertexList(vs.begin() + (v1 - v0), vs.end());
	//vs1.push_back(vs.front());

	const size_t n = vertices.size();
	VertexList vs0;
	for (size_t i = v0; true; i = (i + 1) % n) {
		vs0.push_back(vertices[i]);
		if (i == v1) {
			vs0.insert(vs0.end(), insertVertices.begin(), insertVertices.end());
			break;
		}
	}
	VertexList vs1;
	for (size_t i = v1; true; i = (i + 1) % n) {
		vs1.push_back(vertices[i]);
		if (i == v0) {
			vs1.insert(vs1.end(), insertVertices.rbegin(), insertVertices.rend());
			break;
		}
	}

	Ensures(!vs0.empty());
	Ensures(!vs1.empty());
	Ensures(vs0.size() + vs1.size() == vertices.size() + (2 * insertVertices.size()) + 2);

	auto res = std::pair<Polygon, Polygon>(Polygon(std::move(vs0)), Polygon(std::move(vs1)));
	res.first.simplify();
	res.second.simplify();

	Ensures(res.first.valid);
	Ensures(res.second.valid);
	Ensures(res.first.clockwise == clockwise);
	Ensures(res.second.clockwise == clockwise);

	return res;
}

int Polygon::isInsideAngle(Vector2f a, Vector2f b, Vector2f c, Vector2f p, bool clockwise)
{
	Vector2f u = (a - b).orthoRight();
	Vector2f v = (c - b).orthoLeft();
	if (!clockwise) {
		u = -u;
		v = -v;
	}
	return (u.dot(p - b) > 0 ? 1 : 0) + (v.dot(p - b) > 0 ? 1 : 0);
}

bool Polygon::overlapsEdge(LineSegment segment) const
{
	const size_t n = vertices.size();
	for (size_t i = 0; i < n; ++i) {
		const auto edge = LineSegment(vertices[i], vertices[(i + 1) % n]);
		const auto intersection = segment.intersection(edge);
		if (intersection && !segment.sharesVertexWith(edge)) {
			return true;
		}
	}
	return false;
}

std::optional<std::vector<Polygon>> Polygon::subtract(const Polygon& other) const
{
	if (!convex) {
		throw Exception("Error on polygon subtraction: subtrahend is not convex.", HalleyExceptions::Utils);
	}
	if (!other.convex) {
		throw Exception("Error on polygon subtraction: minuend is not convex.", HalleyExceptions::Utils);
	}
	
	switch (classify(other)) {
	case SATClassification::Separate:
		return {};
	case SATClassification::Overlap:
		return subtractOverlapping(other, true);
	case SATClassification::Contains:
		return subtractContained(other);
	case SATClassification::IsContainedBy:
		return std::vector<Polygon>();
	}

	throw Exception("Unknown polygon SAT classification", HalleyExceptions::Utils);
}

std::vector<Polygon> Polygon::subtractOverlapping(const Polygon& other, bool forceConvexOutput) const
{
	// Based on the paper "Polygon Subtraction in Two or Three Dimensions" by JE Wilson, October 2013
	
	Expects(other.convex); // This method can accept concave polygons, but it requires an additional step outlined in the paper (insert a midpoint in "isolated" edges)
	
	struct VertexInfo {
		Vector2f pos;
		int origId;
		int crossIdx = -1;
		bool processed = false;
		bool outside = false;

		VertexInfo() = default;
		VertexInfo(Vector2f pos, int origId, int crossIdx = -1) : pos(pos), origId(origId), crossIdx(crossIdx) {}
	};

	std::vector<VertexInfo> polyA;
	std::vector<VertexInfo> polyB;

	// Start by filling the data for both polygons
	{
		int i = 0;
		for (auto v: vertices) {
			polyA.emplace_back(v, i++);
		}
	}
	{
		int i = 0;
		for (auto v: other.vertices) {
			polyB.emplace_back(v, i++);
		}
	}

	// Find common vertices
	size_t crossings = 0;
	for (size_t i = 0; i < polyA.size(); ++i) {
		Vector2f a = polyA[i].pos;
		for (size_t j = 0; j < polyB.size(); ++j) {
			Vector2f b = polyB[j].pos;
			if (a.epsilonEquals(b, 0.00001f)) {
				polyA[i].crossIdx = polyB[j].origId;
				polyB[j].crossIdx = polyA[i].origId;
				++crossings;
			}
		}
	}

	// Find intersections and cross-reference them
	for (size_t i = 0; i < polyA.size();) {
		bool inserted = false;
		
		Vector2f a = polyA[i].pos;
		Vector2f b = polyA[(i + 1) % polyA.size()].pos;
		LineSegment segmentA(a, b);

		// Check against the edges of B
		for (size_t j = 0; j < polyB.size(); ++j) {
			Vector2f c = polyB[j].pos;
			Vector2f d = polyB[(j + 1) % polyB.size()].pos;
			LineSegment segmentB(c, d);

			if (!segmentA.sharesVertexWith(segmentB)) {
				const auto intersection = segmentA.intersection(segmentB);
				if (intersection) {
					const int idA = static_cast<int>(polyA.size());
					const int idB = static_cast<int>(polyB.size());
					polyA.insert(polyA.begin() + i + 1, VertexInfo(intersection.value(), idA, idB));
					polyB.insert(polyB.begin() + j + 1, VertexInfo(intersection.value(), idB, idA));
					++crossings;
					inserted = true;
					break;
				}
			}
		}

		if (!inserted) {
			 ++i;
		}
	}

	//assert(crossings >= 2);
	if (crossings < 2) {
		return std::vector<Polygon>();
	}

	// Re-map the cross references and determine if outside the other
	const float epsilon = 0.001f;
	for (auto& v: polyA) {
		if (v.crossIdx != -1) {
			const auto iter = std::find_if(polyB.begin(), polyB.end(), [&] (const VertexInfo& o) { return o.origId == v.crossIdx; });
			v.crossIdx = static_cast<int>(iter - polyB.begin());
		}
		v.outside = !other.isPointInside(v.pos) && !other.isPointOnEdge(v.pos, epsilon);
	}
	for (auto& v: polyB) {
		if (v.crossIdx != -1) {
			const auto iter = std::find_if(polyA.begin(), polyA.end(), [&] (const VertexInfo& o) { return o.origId == v.crossIdx; });
			v.crossIdx = static_cast<int>(iter - polyA.begin());
		}
		v.outside = !isPointInside(v.pos) && !isPointOnEdge(v.pos, epsilon);
	}

	auto findFirstUnusedOutsidePoint = [&] (const std::vector<VertexInfo>& vs) -> std::optional<size_t>
	{
		const auto iter = std::find_if(vs.begin(), vs.end(), [&] (const VertexInfo& v) { return v.outside && !v.processed; });
		if (iter == vs.end()) {
			return {};
		}
		return iter - vs.begin();
	};

	// Run the splitting algorithm
	const int nextOffset = clockwise == other.clockwise ? -1 : 1;
	std::vector<Polygon> result;

	while (true) {
		const auto startPoint = findFirstUnusedOutsidePoint(polyA);
		if (!startPoint) {
			// Done!
			break;
		}

		std::vector<Vector2f> curOutput;

		for (size_t idxA = startPoint.value(); idxA != startPoint.value() || curOutput.empty(); idxA = (idxA + 1) % polyA.size()) {
			auto& curVert = polyA[idxA];
			curVert.processed = true;
			curOutput.push_back(curVert.pos);

			const bool isCrossingPoint = curVert.crossIdx != -1; // TODO: also check for special case

			if (isCrossingPoint) {
				size_t idxB = curVert.crossIdx;
				while (true) {
					idxB = (idxB + polyB.size() + nextOffset) % polyB.size(); // Increment with bi-directional unsigned wraparound
					auto& curOtherVert = polyB[idxB];
					curOutput.push_back(curOtherVert.pos);

					const bool isOtherCrossingPoint = curOtherVert.crossIdx != -1; // TODO: also check for special case
					if (isOtherCrossingPoint) {
						idxA = static_cast<size_t>(curOtherVert.crossIdx);
						break;
					}
				}
			}
		}

		// Output polygons
		auto poly = Polygon(std::move(curOutput));
		if (forceConvexOutput) {
			poly.splitIntoConvex(result);
		} else {
			result.emplace_back(std::move(poly));
		}
	}

	return result;
}

std::vector<Polygon> Polygon::subtractContained(const Polygon& other) const
{
	// The idea here is to find the chord that gets closest to the other polygon and this polygon in two, then subtract other from both
	// If the chord goes right through the other polygon, there's nothing else to be done
	// If, however, it doesn't, then a new vertex has to be inserted into that chord to "bend" it so it goes inside "other"...
	// ...This means that one of the two halves is now concave, so convert them all to convex, then subtract them.
	
	std::pair<size_t, size_t> bestChord = {0, 0};
	float bestChordDistance = std::numeric_limits<float>::infinity();
	const size_t n = vertices.size();

	for (size_t i = 0; i < n; ++i) {
		for (size_t j = i + 1; j < n; ++j) {
			const float distance = other.getDistanceTo(Line(vertices[i], (vertices[j] - vertices[i]).normalized()));
			if (distance < bestChordDistance) {
				bestChordDistance = distance;
				bestChord = std::make_pair(i, j);
			}
		}
	}

	assert(bestChord.second != bestChord.first);

	if (bestChordDistance < 0) {
		// Found a chord that goes right through the polygon, split there
		std::pair<Polygon, Polygon> polys = doSplit(bestChord.first, bestChord.second, {});
		std::vector<Polygon> res0 = polys.first.subtract(other).value();
		std::vector<Polygon> res1 = polys.second.subtract(other).value();
		for (auto& r: res1) {
			res0.emplace_back(std::move(r));
		}
		return res0;
	} else {
		// Split along the chord, but insert an extra vertex
		Vector2f extraVertex = other.getCentre();
		std::pair<Polygon, Polygon> polys = doSplit(bestChord.first, bestChord.second, gsl::span<const Vector2f>(&extraVertex, 1));
		std::vector<Polygon> splitConvexPolys;
		polys.first.splitIntoConvex(splitConvexPolys);
		polys.second.splitIntoConvex(splitConvexPolys);

		std::vector<Polygon> result;
		for (auto& poly: splitConvexPolys) {
			auto subResult = poly.subtract(other);
			if (subResult) {
				for (auto& p: subResult.value()) {
					result.emplace_back(std::move(p));
				}
			} else {
				result.emplace_back(std::move(poly));
			}
		}
		
		return result;
	}
}

Polygon Polygon::makePolygon(Vector2f origin, float w, float h)
{
	const float x = origin.x;
	const float y = origin.y;
	VertexList list;
	list.push_back(Vertex(x, y));
	list.push_back(Vertex(x+w, y));
	list.push_back(Vertex(x+w, y+h));
	list.push_back(Vertex(x, y+h));
	return Polygon(list);
}

void Polygon::setVertices(VertexList _vertices)
{
	vertices = std::move(_vertices);
	realize();
}

Vector2f Polygon::getCentre() const
{
	return circle.getCentre();
}

void Polygon::translate(Vector2f offset)
{
	for (auto& v: vertices) {
		v += offset;
	}
	realize();
}


Polygon::CollisionResult Polygon::getCollisionWithSweepingCircle(Vector2f p0, float radius, Vector2f moveDir, float moveLen) const
{
	CollisionResult result;

	// This is used to grow AABBs to check if p0 is inside
	// If this coarse test fails, the sweep shouldn't overlap the polygon
	const float border = radius + (moveLen * std::max(std::abs(moveDir.x), std::abs(moveDir.y)));
	if (!getAABB().grow(border).contains(p0)) {
		result.fastFail = true;
		return result;
	}

	const auto submit = [&] (std::optional<std::pair<float, Vector2f>> c)
	{
		if (c && c->first < moveLen) {
			if (!result.collided || c->first < result.distance) {
				result.collided = true;
				result.distance = c->first;
				result.normal = c->second;
			}
		}
	};
	
	const auto ray = Ray(p0, moveDir);

	for (size_t i = 0; i < vertices.size(); ++i) {
		// For each line segment in the polygon...
		const Vector2f a = vertices[i];
		const Vector2f b = vertices[(i + 1) % vertices.size()];

		// We expand the line segment into a rounded capsule.
		// It's now two circles (one centred at each vertex) and two line segments (connecting the circles)
		// Checking collision of this capsule against the centre of the circle is isomorphic to the original problem, but easier

		// Check circles
		// Only check one vertex, "b" will be checked by another iteration
		submit(ray.castCircle(a, radius));

		// Check segments
		// One of the two line segments (facing away) is not needed, so we only test two circles and one segment
		Vector2f offset = (a - b).normalized().orthoLeft() * radius;
		if (offset.dot(moveDir) > 0) {
			offset = -offset;
		}
		submit(ray.castLineSegment(a + offset, b + offset));
	}
	
	return result;
}

Polygon::CollisionResult Polygon::getCollisionWithSweepingEllipse(Vector2f p0, Vector2f radius, Vector2f moveDir, float moveLen) const
{
	// This is the same algorithm as above, but we scale everything so the ellipse becomes a circle
	CollisionResult result;
	
	// This is used to grow AABBs to check if p0 is inside
	// If this coarse test fails, the sweep shouldn't overlap the polygon
	const float border = std::max(radius.x, radius.y) + (moveLen * std::max(std::abs(moveDir.x), std::abs(moveDir.y)));
	if (!getAABB().grow(border).contains(p0)) {
		result.fastFail = true;
		return result;
	}

	const auto localRadius = radius.x;
	const auto scale = radius.x / radius.y;
	const auto transformation = Vector2f(1.0f, scale);

	const auto localMove = moveDir * transformation * moveLen;
	const auto localMoveLen = localMove.length();
	const auto localMoveDir = localMove.normalized();
	const auto localP0 = p0 * transformation;
	const auto ray = Ray(localP0, localMoveDir);

	float bestLen = localMoveLen;
	const auto submit = [&] (std::optional<std::pair<float, Vector2f>> c)
	{
		if (c) {
			const float lenToCol = c->first;
			if (lenToCol < bestLen) {
				result.collided = true;
				result.distance = c->first;
				result.normal = c->second;
				bestLen = lenToCol;
			}
		}
	};

	for (size_t i = 0; i < vertices.size(); ++i) {
		// For each line segment in the polygon...
		const Vector2f a = vertices[i] * transformation;
		const Vector2f b = vertices[(i + 1) % vertices.size()] * transformation;

		// We expand the line segment into a rounded capsule.
		// It's now two circles (one centred at each vertex) and two line segments (connecting the circles)
		// Checking collision of this capsule against the centre of the circle is isomorphic to the original problem, but easier

		// Check circle
		// Only check one vertex, "b" will be checked by another iteration
		submit(ray.castCircle(a, localRadius));

		// Check segments
		// One of the two line segments (facing away) is not needed, so we only test two circles and one segment
		Vector2f offset = (a - b).normalized().orthoLeft() * localRadius;
		if (offset.dot(localMoveDir) > 0) {
			offset = -offset;
		}
		submit(ray.castLineSegment(a + offset, b + offset));
	}

	if (result.collided) {
		// Transform the results back to global space
		result.distance *= moveLen / localMoveLen;

		// This is a multiply instead of the divide you might expect
		// The correct operation here is (norm.orthoLeft() / transform).orthoRight().normalized()
		// But this is equivalent and faster
		result.normal = (result.normal * transformation).normalized();
	}
	return result;
}

float Polygon::getDistanceTo(const Line& line) const
{
	const auto n = line.dir.orthoRight();
	const auto p = line.origin.dot(n);
	const auto range = project(n);
	if (range.contains(p)) {
		return -std::min(p - range.start, range.end - p);
	} else {
		return std::max(range.start - p, p - range.end);
	}
}

bool Polygon::operator==(const Polygon& other) const
{
	return vertices == other.vertices;
}

bool Polygon::operator!=(const Polygon& other) const
{
	return vertices != other.vertices;
}

ConfigNode Polygon::toConfigNode() const
{
	return ConfigNode(getVertices());
}

ConfigNode ConfigNodeSerializer<Polygon>::serialize(const Polygon& polygon, const ConfigNodeSerializationContext&)
{
	return polygon.toConfigNode();
}

Polygon ConfigNodeSerializer<Polygon>::deserialize(const ConfigNodeSerializationContext&, const ConfigNode& node) 
{
	return Polygon(node);
}

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


#pragma once

#include <halley/data_structures/vector.h>
#include "vector2.h"
#include "rect.h"
#include "circle.h"
#include "line.h"
#include "halley/bytes/config_node_serializer_base.h"

namespace Halley {
	class Ray;
	class LineSegment;

	using Vertex = Vector2f;
	using VertexList = Vector<Vertex>;

	class Polygon {
	public:
		struct CollisionResult {
			Vector2f normal;
			float distance = 0;
			bool collided = false;
			bool fastFail = false;
		};

		enum class SATClassification {
			Separate,
			Overlap,
			Contains,
			IsContainedBy
		};
		
		Polygon();
		Polygon(VertexList vertices);
		explicit Polygon(const ConfigNode& node);

		Polygon(const Polygon& other) = default;
		Polygon(Polygon&& other) noexcept = default;
		Polygon& operator=(const Polygon& other) = default;
		Polygon& operator=(Polygon&& other) noexcept = default;

		static Polygon makePolygon(Vector2f origin, float w, float h);

		bool isPointInside(Vector2f point) const;
		bool isPointOnEdge(Vector2f point, float epsilon) const;

		bool collide(const Polygon &param, Vector2f *translation= nullptr, Vector2f *collisionPoint= nullptr) const;
		Vector2f getClosestPoint(Vector2f p, float anisotropy = 1.0f) const; // All Y coordinates are multiplied by anisotropy

		SATClassification classify(const Polygon& other) const;
		SATClassification classify(const LineSegment& line) const;

		void setVertices(VertexList vertices);
		const VertexList& getVertices() const { return vertices; }
		const size_t getNumSides() const { return vertices.size(); }
		LineSegment getEdge(size_t idx) const;
		std::optional<size_t> findEdge(const LineSegment& edge, float epsilon) const;
		
		void rotate(Angle<float> angle);
		void rotateAndScale(Angle<float> angle, Vector2f scale);
		void scale(Vector2f scale);
		void expand(float amount, float truncateThreshold = 0.0f);
		void invertWinding();

		Polygon convolution(const Polygon& other) const;
		
		bool isConvex() const { return convex; }
		bool isClockwise() const { return clockwise; }
		bool isValid() const { return valid; }

		std::vector<Polygon> splitIntoConvex() const;
		bool splitIntoConvex(std::vector<Polygon>& output) const;
		std::optional<std::vector<Polygon>> subtract(const Polygon& other) const;
		void simplify(float epsilon = 0.0001f);
		std::vector<Polygon> splitConvexIntoMaxSides(size_t maxSides) const;
		std::vector<Polygon> splitConvexByLine(const Line& line) const;

		const Rect4f& getAABB() const { return aabb; }
		const Circle& getBoundingCircle() const { return circle; }
		Vector2f getCentre() const;

		void translate(Vector2f offset);

		// Returns the distance from circlePos, along moveDir, until the collision point, and the collision normal.
		// Only returns a value if a collision is found between start pos and up to move len away
		CollisionResult getCollisionWithSweepingCircle(Vector2f circlePos, float radius, Vector2f moveDir, float moveLen) const;
		CollisionResult getCollisionWithSweepingEllipse(Vector2f circlePos, Vector2f radius, Vector2f moveDir, float moveLen) const;

		float getDistanceTo(const Line& line) const;

		bool operator==(const Polygon& other) const;
		bool operator!=(const Polygon& other) const;

		ConfigNode toConfigNode() const;

		std::optional<size_t> getExitEdge(const Ray& ray) const;

		float getArea() const { return area; }

	private:
		Circle circle;
		VertexList vertices;
		bool convex = false;
		bool clockwise = false;
		bool valid = false;
		float area = 0;
		Rect4f aabb;

		bool isPointInsideConvex(Vector2f point) const;
		bool isPointInsideConcave(Vector2f point) const;

		bool collideConvex(const Polygon &param, Vector2f *translation= nullptr, Vector2f *collisionPoint= nullptr) const;

		Range<float> project(Vector2f axis) const;
		void unproject(const Vector2f &axis,const float point,Vector<Vector2f> &ver) const;
		void realize();
		void checkConvex();

		// Split by inserting a new edge between v0 and v1
		std::pair<Polygon, Polygon> doSplit(size_t v0, size_t v1, gsl::span<const Vector2f> insertVertices) const;

		void doSplitConvexIntoMaxSides(size_t maxSides, std::vector<Polygon>& output) const;

		// Angle is ABC (B in the middle). Checks against semi-planes defined by AB and BC
		// Returns:
		// 0 if outside
		// 1 if "concave inside" (inside one of the semi-planes, but not both)
		// 2 if "convex inside" (inside both semi-planes)
		static int isInsideAngle(Vector2f a, Vector2f b, Vector2f c, Vector2f p, bool clockwise);
		bool overlapsEdge(LineSegment segment) const;

		std::optional<std::vector<Polygon>> subtractOverlapping(const Polygon& other, bool forceConvexOutput) const;
		std::vector<Polygon> subtractContained(const Polygon& other) const;
	};

	template<>
	class ConfigNodeSerializer<Polygon> {
	public:
		ConfigNode serialize(const Polygon& polygon, const ConfigNodeSerializationContext&);
		Polygon deserialize(const ConfigNodeSerializationContext&, const ConfigNode& node);
	};
}

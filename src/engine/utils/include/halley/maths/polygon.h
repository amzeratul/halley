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
#include "halley/data_structures/maybe.h"
#include "circle.h"

namespace Halley {

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
		
		Polygon();
		Polygon(VertexList vertices);

		static Polygon makePolygon(Vector2f origin, float w, float h);

		bool isPointInsideConvex(Vector2f point) const;
		bool isPointInside(Vector2f point) const;
		bool overlaps(const Polygon &param, Vector2f *translation= nullptr, Vector2f *collisionPoint= nullptr) const;

		void setVertices(const VertexList& vertices);
		const VertexList& getVertices() const { return vertices; }
		
		void rotate(Angle<float> angle);
		void rotateAndScale(Angle<float> angle, Vector2f scale);
		bool isClockwise() const;

		Rect4f getAABB() const;

		void translate(Vector2f offset);

		// Returns the distance from circlePos, along moveDir, until the collision point, and the collision normal.
		// Only returns a value if a collision is found between start pos and up to move len away
		CollisionResult getCollisionWithSweepingCircle(Vector2f circlePos, float radius, Vector2f moveDir, float moveLen) const;
		CollisionResult getCollisionWithSweepingEllipse(Vector2f circlePos, Vector2f radius, Vector2f moveDir, float moveLen) const;

	private:
		Circle circle;
		VertexList vertices;
		Rect4f aabb;

		void project(const Vector2f &axis,float &min,float &max) const;
		void unproject(const Vector2f &axis,const float point,Vector<Vector2f> &ver) const;
		void realize();
	};
}

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

namespace Halley {

	using Vertex = Vector2f;
	using VertexList = Vector<Vertex>;

	class Polygon {
	public:
		Polygon();
		Polygon(const VertexList& vertices, Vertex origin=Vertex());

		static Polygon makePolygon(Vector2f origin, float w, float h);

		bool isPointInside(const Vector2f &point) const;
		bool overlaps(const Polygon &param, Vector2f *translation= nullptr, Vector2f *collisionPoint= nullptr) const;

		void setVertices(const VertexList& vertices);
		void setOrigin(const Vertex& _origin) { origin = _origin; }
		const VertexList& getVertices() const { return vertices; }
		const Vertex& getOrigin() const { return origin; }
		void rotate(Angle<float> angle);
		void rotateAndScale(Angle<float> angle, Vector2f scale);
		bool isClockwise() const;
		float getRadius() const;

		Rect4f getAABB() const;

		// Returns the distance from circlePos, along moveDir, until the collision point.
		// Only returns a float if a collision is found between start pos and up to move len away
		Maybe<float> getCollisionWithSweepingCircle(Vector2f circlePos, float radius, Vector2f moveDir, float moveLen) const;

	private:
		float outerRadius;
		VertexList vertices;
		Vertex origin;
		Rect4f aabb;

		void project(const Vector2f &axis,float &min,float &max) const;
		void unproject(const Vector2f &axis,const float point,Vector<Vector2f> &ver) const;
		void realize();
	};
}

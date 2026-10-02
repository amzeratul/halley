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

#include "polygon.h"
#include "vector2.h"

namespace Halley {
	class Base2D {
	public:
		constexpr Base2D() = default;

		constexpr Base2D(Vector2f u, Vector2f v)
			: u(u), v(v)
		{
			const float det = 1.0f / u.cross(v);
			invU = det * Vector2f(v.y, -u.y);
			invV = det * Vector2f(-v.x, u.x);
		}

		explicit Base2D(const ConfigNode& node);

		constexpr Vector2f transform(Vector2f point) const
		{
			return transform(point, u, v);
		}

		constexpr Vector2f inverseTransform(Vector2f point) const
		{
			return transform(point, invU, invV);
		}
		constexpr static Vector2f transform(Vector2f point, Vector2f u, Vector2f v)
		{
			return point.x * u + point.y * v;
		}

		Polygon transform(const Polygon& poly) const;
		Polygon inverseTransform(const Polygon& poly) const;

		Base2D getInverse() const;

		ConfigNode toConfigNode() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		Vector2f u, v;
		Vector2f invU, invV;

		Base2D(Vector2f u, Vector2f v, Vector2f invU, Vector2f invV);
	};
}
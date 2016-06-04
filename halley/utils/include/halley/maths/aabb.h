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
#include "vector2d.h"

namespace Halley {

	class AABB {
	public:
		AABB();
		AABB(Vector2f p1, Vector2f p2);

		bool overlaps(const AABB& p, Vector2f delta=Vector2f()) const;
		bool isPointInside(Vector2f p) const;
		void set(Vector2f p1, Vector2f p2);

	private:
		Vector2f p1, p2;
	};

}

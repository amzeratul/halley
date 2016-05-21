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

#include "halley/maths/vector2d.h"
#include "halley/maths/angle.h"
#include "halley/maths/rect.h"
#include "halley/maths/matrix4.h"

namespace Halley {

	class RenderTarget;

	class Camera {
	public:
		Camera();
		Camera(Vector2f pos, Vector2f area, Angle1f angle=Angle1f::fromDegrees(0));

		void setPosition(Vector2f pos);
		void setViewArea(Vector2f area);
		void setAngle(Angle1f angle);
		void setZoom(float zoom);

		Vector2f getPosition() const { return pos; }
		Vector2f getViewArea() const { return area; }
		Rect4f getViewRect() const;
		Angle1f getAngle() const { return angle; }
		float getZoom() const { return zoom; }

		Vector2f screenToWorld(Vector2f p, Rect4f viewport) const;
		Vector2f worldToScreen(Vector2f p, Rect4f viewport) const;

		Matrix4f getProjection() const { return projection; }

		void updateProjection();

	private:
		Vector2f pos;
		Vector2f area;
		Matrix4f projection;
		Angle1f angle;
		float zoom;
	};
}

#pragma once

#include "graphics/sprite/sprite.h"
#include "graphics/text/text_renderer.h"

namespace Halley {
	class UIStyle {
	public:
		Sprite windowBackground;
		Vector4s windowInnerBorder;

		Sprite buttonNormal;
		Sprite buttonHover;
		Sprite buttonDown;
		TextRenderer buttonLabel;
		Vector4s buttonInnerBorder;

		TextRenderer label;
	};
}

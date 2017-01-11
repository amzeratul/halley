#pragma once

#include "graphics/sprite/sprite.h"
#include "graphics/text/text_renderer.h"

namespace Halley {
	class AudioClip;

	class UIStyle {
	public:
		Sprite windowBackground;
		Vector4f windowInnerBorder;

		Sprite buttonNormal;
		Sprite buttonHover;
		Sprite buttonDown;
		TextRenderer buttonLabel;
		Vector4f buttonInnerBorder;
		std::shared_ptr<const AudioClip> buttonHoverSound;
		std::shared_ptr<const AudioClip> buttonDownSound;
		std::shared_ptr<const AudioClip> buttonUpSound;

		TextRenderer label;
	};
}

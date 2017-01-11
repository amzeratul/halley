#pragma once

#include "graphics/sprite/sprite.h"
#include "graphics/text/text_renderer.h"
#include "halley/audio/audio_clip.h"

namespace Halley {
	class UIStyle {
	public:
		Sprite windowBackground;
		Vector4f windowInnerBorder;

		Sprite buttonNormal;
		Sprite buttonHover;
		Sprite buttonDown;
		TextRenderer buttonLabel;
		Vector4f buttonInnerBorder;
		std::shared_ptr<AudioClip> buttonHoverSound;
		std::shared_ptr<AudioClip> buttonDownSound;
		std::shared_ptr<AudioClip> buttonUpSound;

		TextRenderer label;
	};
}

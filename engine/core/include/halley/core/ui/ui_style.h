#pragma once

#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/text/text_renderer.h"

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

		Sprite inputBox;
		TextRenderer inputLabel;
		TextRenderer inputLabelGhost;

		TextRenderer label;

		Sprite dialogueBackground;
		Sprite dialogueTitleBar;
		TextRenderer dialogueTitleLabel;

		Sprite horizontalDiv;
		Sprite verticalDiv;

		Sprite checkboxNormal;
		Sprite checkboxChecked;
		Sprite checkboxNormalHover;
		Sprite checkboxCheckedHover;
	};
}

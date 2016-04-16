#pragma once
#include "animation.h"
#include "sprite_sheet.h"

namespace Halley
{
	class Sprite;

	class AnimationPlayer
	{
	public:
		void setAnimation(std::shared_ptr<Animation> animation);
		void update(Time time);

		void updateSprite(Sprite& sprite) const;

	private:
		std::shared_ptr<Animation> animation;
		const SpriteSheetEntry* spriteSheet;
	};
}

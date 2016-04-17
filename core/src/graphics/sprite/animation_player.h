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
		void setSequence(String sequence);
		void setDirection(String direction);

		void update(Time time);

		void updateSprite(Sprite& sprite) const;

	private:
		void resolveSprite();

		std::shared_ptr<Animation> animation;
		const SpriteSheetEntry* spriteData = nullptr;

		bool dirty;

		float seqFPS;
		size_t seqLen;

		int curFrame;
		float curTime;
		const AnimationSequence* curSeq;
		const AnimationDirection* curDir;
	};
}

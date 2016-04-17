#pragma once
#include "animation.h"
#include "sprite_sheet.h"

namespace Halley
{
	class Sprite;

	class AnimationPlayer
	{
	public:
		explicit AnimationPlayer(std::shared_ptr<Animation> animation = std::shared_ptr<Animation>(), String sequence = "default", String direction = "default");
		void setAnimation(std::shared_ptr<Animation> animation, String sequence = "default", String direction = "default");
		void setSequence(String sequence);
		void setDirection(int direction);
		void setDirection(String direction);

		void update(Time time);

		void updateSprite(Sprite& sprite) const;

	private:
		void resolveSprite();

		std::shared_ptr<Animation> animation;
		const SpriteSheetEntry* spriteData = nullptr;

		bool dirty;

		bool seqLooping;
		float seqFPS;
		size_t seqLen;
		Time seqTimeLen;

		int curFrame;
		Time curTime;
		const AnimationSequence* curSeq;
		const AnimationDirection* curDir;
	};
}

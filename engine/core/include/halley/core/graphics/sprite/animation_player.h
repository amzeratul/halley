#pragma once
#include "animation.h"
#include "sprite_sheet.h"
#include <halley/time/halleytime.h>

namespace Halley
{
	class Sprite;

	class AnimationPlayer
	{
	public:
		explicit AnimationPlayer(std::shared_ptr<Animation> animation = std::shared_ptr<Animation>(), String sequence = "default", String direction = "default");

		void playOnce(String sequence);

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

		const AnimationSequence* curSeq;
		const AnimationDirection* curDir;

		Time seqTimeLen;
		Time curTime;

		size_t seqLen;

		int dirId;
		int curFrame;
		float seqFPS;

		bool dirty;
		bool seqLooping;
		bool seqNoFlip;
		bool dirFlip;

		mutable bool hasUpdate = true;
	};
}

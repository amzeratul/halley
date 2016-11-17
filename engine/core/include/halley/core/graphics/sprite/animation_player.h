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
		explicit AnimationPlayer(std::shared_ptr<const Animation> animation = std::shared_ptr<const Animation>(), String sequence = "default", String direction = "default");

		AnimationPlayer& playOnce(String sequence);

		AnimationPlayer& setAnimation(std::shared_ptr<const Animation> animation, String sequence = "default", String direction = "default");
		AnimationPlayer& setSequence(String sequence);
		AnimationPlayer& setDirection(int direction);
		AnimationPlayer& setDirection(String direction);

		AnimationPlayer& setApplyPivot(bool apply);

		void update(Time time);

		void updateSprite(Sprite& sprite) const;

		AnimationPlayer& setMaterialOverride(std::shared_ptr<Material> material);
		std::shared_ptr<Material> getMaterialOverride() const;
		std::shared_ptr<const Material> getMaterial() const;

		bool isPlaying() const;
		const String& getCurrentSequenceName() const;
		Time getCurrentSequenceTime() const;
		int getCurrentSequenceFrame() const;
		
	private:
		void resolveSprite();

		void onSequenceStarted();
		void onSequenceDone();

		std::shared_ptr<Material> materialOverride;
		std::shared_ptr<const Animation> animation;
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
		bool playing = false;

		bool applyPivot = true;

		mutable bool hasUpdate = true;
	};
}

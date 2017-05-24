#pragma once
#include "animation.h"
#include "sprite_sheet.h"
#include <halley/time/halleytime.h>
#include "halley/data_structures/maybe.h"

namespace Halley
{
	class Sprite;

	class AnimationPlayer
	{
	public:
		explicit AnimationPlayer(std::shared_ptr<const Animation> animation = std::shared_ptr<const Animation>(), const String& sequence = "default", const String& direction = "default");

		AnimationPlayer& playOnce(const String& sequence);

		AnimationPlayer& setAnimation(std::shared_ptr<const Animation> animation, const String& sequence = "default", const String& direction = "default");
		AnimationPlayer& setSequence(const String& sequence);
		AnimationPlayer& setDirection(int direction);
		AnimationPlayer& setDirection(const String& direction);
		bool trySetSequence(const String& sequence);

		AnimationPlayer& setApplyPivot(bool apply);

		void update(Time time);

		void updateSprite(Sprite& sprite) const;

		AnimationPlayer& setMaterialOverride(std::shared_ptr<Material> material);
		std::shared_ptr<Material> getMaterialOverride() const;
		std::shared_ptr<const Material> getMaterial() const;

		bool isPlaying() const;
		String getCurrentSequenceName() const;
		Time getCurrentSequenceTime() const;
		int getCurrentSequenceFrame() const;

		String getCurrentDirectionName() const;

		AnimationPlayer& setPlaybackSpeed(float value);
		float getPlaybackSpeed() const;

		const Animation& getAnimation() const;
		bool hasAnimation() const;

		void setOffsetPivot(Vector2f offset);

	private:
		void resolveSprite();

		void onSequenceStarted();
		void onSequenceDone();

		std::shared_ptr<Material> materialOverride;
		std::shared_ptr<const Animation> animation;
		const SpriteSheetEntry* spriteData = nullptr;

		const AnimationSequence* curSeq = nullptr;
		const AnimationDirection* curDir = nullptr;

		Time seqTimeLen;
		Time curTime;

		size_t seqLen;

		int dirId;
		int curFrame;
		float seqFPS;
		float playbackSpeed = 1.0f;

		Vector2f offsetPivot;

		bool dirty;
		bool seqLooping;
		bool seqNoFlip;
		bool dirFlip;
		bool playing = false;

		bool applyPivot = true;

		mutable bool hasUpdate = true;
	};
}

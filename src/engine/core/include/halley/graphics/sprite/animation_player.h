#pragma once
#include "animation.h"
#include "sprite_sheet.h"
#include <halley/time/halleytime.h>
#include "halley/data_structures/maybe.h"
#include "halley/file_formats/config_file.h"
#include "halley/bytes/config_node_serializer.h"

namespace Halley
{
	class Sprite;

	class AnimationPlayer
	{
	public:
		explicit AnimationPlayer(std::shared_ptr<const Animation> animation = std::shared_ptr<const Animation>(), const String& sequence = "default", const String& direction = "default");

		AnimationPlayer& playOnce(const String& sequence, const std::optional<String>& nextLoopingSequence = {}, bool reverse = false);
		AnimationPlayer& stop();

		AnimationPlayer& setAnimation(std::shared_ptr<const Animation> animation, const String& sequence = "default", const String& direction = "default");
		AnimationPlayer& setSequence(const String& sequence);
		AnimationPlayer& setDirection(int direction);
		AnimationPlayer& setDirection(const String& direction);
		bool trySetSequence(const String& sequence);

		AnimationPlayer& setApplyPivot(bool apply);
		bool isApplyingPivot() const;

		void update(Time time);
		void updateSprite(Sprite& sprite) const;

		AnimationPlayer& setMaterialOverride(std::shared_ptr<const Material> material);
		std::shared_ptr<const Material> getMaterialOverride() const;
		std::shared_ptr<const Material> getMaterial() const;
		void setApplyMaterial(bool apply);
		void setReversePlaying(bool reverse);
		bool isApplyingMaterial() const;

		bool isPlaying() const;
		const String& getCurrentSequenceName() const;
		Time getCurrentSequenceTime() const;
		int getCurrentSequenceFrame() const;
		Time getCurrentSequenceFrameTime() const;
		int getCurrentSequenceLoopCount() const;
		int getCurrentSequenceLength() const;

		String getCurrentDirectionName() const;
		int getCurrentDirectionId() const;
		bool isFlipped() const;

		AnimationPlayer& setPlaybackSpeed(float value);
		float getPlaybackSpeed() const;

		const Animation& getAnimation() const;
		std::shared_ptr<const Animation> getAnimationPtr() const;
		bool hasAnimation() const;

		AnimationPlayer& setOffsetPivot(Vector2f offset);

		void syncWith(const AnimationPlayer& masterAnimator, bool hideIfNotSynchronized);
		void setState(const String& sequenceName, const String& directionName, int currentFrame, Time currentFrameTime, bool hideIfNotSynchronized);
		void setTiming(int currentFrame, Time currentFrameTime);
		void stepFrames(int amount);

		std::optional<Vector2i> getCurrentActionPoint(const String& actionPointId) const;

	private:
		void resolveSprite();
		void updateResourceIfNeeded() const;
		void doUpdateResource();

		void onSequenceStarted();
		void onSequenceDone();

		std::shared_ptr<const Material> materialOverride;
		std::shared_ptr<const Animation> animation;
		const SpriteSheetEntry* spriteData = nullptr;

		const AnimationFrame* curFrame = nullptr;
		const AnimationSequence* curSeq = nullptr;
		const AnimationDirection* curDir = nullptr;

		std::optional<String> nextSequence = {};
		
		String curSeqName;
		String curDirName;
		ResourceObserver observer;

		Time curSeqTime;
		Time curFrameTime;

		size_t seqLen;

		int dirId = 0;
		int curFrameN = 0;
		int curLoopCount = 0;
		float playbackSpeed = 1.0f;

		Vector2f offsetPivot;

		bool dirty;
		bool seqLooping;
		bool seqNoFlip;
		bool dirFlip;
		bool playing = false;
		bool reverse = false;
		std::optional<bool> visibleOverride;

		bool applyPivot = true;
		bool applyMaterial = true;

		mutable bool hasUpdate = true;
	};

	class AnimationPlayerLite {
	public:
		explicit AnimationPlayerLite(std::shared_ptr<const Animation> animation = std::shared_ptr<const Animation>(), const String& sequence = "default", const String& direction = "default");

		AnimationPlayerLite& setAnimation(std::shared_ptr<const Animation> animation, const String& sequence = "default", const String& direction = "default");
		AnimationPlayerLite& setSequence(const String& sequence);
		AnimationPlayerLite& setDirection(int direction);
		AnimationPlayerLite& setDirection(const String& direction);

		void update(Time time, Sprite& sprite);

	private:
		std::shared_ptr<const Animation> animation;
		OptionalLite<int> curSeqIdx;
		float curTime = 0;
		int curFrame = -1;
		int curDir = 0;
	};

	class Resources;

	template<>
	class ConfigNodeSerializer<AnimationPlayer> {
	public:
		ConfigNode serialize(const AnimationPlayer& player, const EntitySerializationContext& context);
		AnimationPlayer deserialize(const EntitySerializationContext& context, const ConfigNode& node);
	};
}

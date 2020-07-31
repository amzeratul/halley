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

		AnimationPlayer& playOnce(const String& sequence, const std::optional<String>& nextLoopingSequence = {});

		AnimationPlayer& setAnimation(std::shared_ptr<const Animation> animation, const String& sequence = "default", const String& direction = "default");
		AnimationPlayer& setSequence(const String& sequence);
		AnimationPlayer& setDirection(int direction);
		AnimationPlayer& setDirection(const String& direction);
		bool trySetSequence(const String& sequence);

		AnimationPlayer& setApplyPivot(bool apply);
		bool isApplyingPivot() const;

		void update(Time time);

		void updateSprite(Sprite& sprite) const;

		AnimationPlayer& setMaterialOverride(std::shared_ptr<Material> material);
		std::shared_ptr<Material> getMaterialOverride() const;
		std::shared_ptr<const Material> getMaterial() const;

		bool isPlaying() const;
		String getCurrentSequenceName() const;
		Time getCurrentSequenceTime() const;
		int getCurrentSequenceFrame() const;
		int getCurrentSequenceLoopCount() const;

		String getCurrentDirectionName() const;

		AnimationPlayer& setPlaybackSpeed(float value);
		float getPlaybackSpeed() const;

		const Animation& getAnimation() const;
		std::shared_ptr<const Animation> getAnimationPtr() const;
		bool hasAnimation() const;

		AnimationPlayer& setOffsetPivot(Vector2f offset);

		void syncWith(const AnimationPlayer& masterAnimator);
		void stepFrames(int amount);

	private:
		void resolveSprite();

		void onSequenceStarted();
		void onSequenceDone();

		void updateIfNeeded();

		std::shared_ptr<Material> materialOverride;
		std::shared_ptr<const Animation> animation;
		const SpriteSheetEntry* spriteData = nullptr;

		const AnimationSequence* curSeq = nullptr;
		const AnimationDirection* curDir = nullptr;

		std::optional<String> nextSequence = {};
		
		String curSeqName;
		String curDirName;
		ResourceObserver observer;

		Time curSeqTime;
		Time curFrameTime;
		Time curFrameLen;

		size_t seqLen;

		int dirId;
		int curFrame;
		int curLoopCount = 0;
		float playbackSpeed = 1.0f;

		Vector2f offsetPivot;

		bool dirty;
		bool seqLooping;
		bool seqNoFlip;
		bool dirFlip;
		bool playing = false;
		std::optional<bool> visibleOverride;

		bool applyPivot = true;

		mutable bool hasUpdate = true;
	};

	class Resources;

	template<>
	class ConfigNodeSerializer<AnimationPlayer> {
	public:
		ConfigNode serialize(const AnimationPlayer& player, ConfigNodeSerializationContext& context);
		AnimationPlayer deserialize(ConfigNodeSerializationContext& context, const ConfigNode& node);
	};
}

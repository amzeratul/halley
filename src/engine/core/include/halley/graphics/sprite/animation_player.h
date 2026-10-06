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
		using AnimationPlayId = uint32_t;

		explicit AnimationPlayer(std::shared_ptr<const Animation> animation = std::shared_ptr<const Animation>(), std::string_view sequence = "default", std::string_view direction = "default");

		AnimationPlayId playOnce(std::string_view sequence, const std::optional<String>& nextLoopingSequence = std::nullopt, bool reverse = false, std::optional<int> startFrame = {});
		AnimationPlayId stop();

		AnimationPlayId setAnimation(std::shared_ptr<const Animation> animation, std::string_view sequence = "default", std::string_view direction = "default");
		AnimationPlayId setSequence(std::string_view sequence);
		void setDirection(int direction);
		void setDirection(std::string_view direction);
		bool trySetSequence(std::string_view sequence);

		AnimationPlayId getCurrentPlayId() const;

		void setApplyPivot(bool apply);
		bool isApplyingPivot() const;

		void update(Time time);
		void updateSprite(Sprite& sprite) const;
		bool isActiveAnimation() const;
		bool hasSpriteUpdate() const;

		void setMaterialOverride(std::shared_ptr<const Material> material);
		std::shared_ptr<const Material> getMaterialOverride() const;
		std::shared_ptr<const Material> getMaterial() const;
		void setApplyMaterial(bool apply);
		void setReversePlaying(bool reverse);
		bool isPlayingReverse();
		bool isApplyingMaterial() const;

		bool isPlaying() const;
		const String& getCurrentSequenceName() const;
		int getCurrentSequenceId() const;
		Time getCurrentSequenceTime() const;
		int getCurrentSequenceFrame() const;
		Time getCurrentSequenceFrameTime() const;
		int getCurrentSequenceLoopCount() const;
		int getCurrentSequenceLength() const;

		const String& getCurrentDirectionName() const;
		int getCurrentDirectionId() const;
		bool isFlipped() const;

		void setPlaybackSpeed(float value);
		float getPlaybackSpeed() const;

		const Animation& getAnimation() const;
		std::shared_ptr<const Animation> getAnimationPtr() const;
		bool hasAnimation() const;

		void setOffsetPivot(Vector2f offset);

		void syncWith(const AnimationPlayer& masterAnimator, bool hideIfNotSynchronized);
		void setState(const String& sequenceName, const String& directionName, int currentFrame, Time currentFrameTime, bool hideIfNotSynchronized);
		void setTiming(int currentFrame, Time currentFrameTime);
		void stepFrames(int amount);

		void setVisibleOverride(std::optional<bool> visible);
		std::optional<bool> getVisibleOverride() const;

		std::optional<Vector2i> getCurrentActionPoint(const String& actionPointId) const;
		
		void feedToHasher(Hash::Hasher& hasher) const;

	private:
		void resolveSprite();
		void updateResourceIfNeeded() const;
		void doUpdateResource();

		void onSequenceStarted();
		void onSequenceDone();

		uint32_t seqLen = 0;
		bool dirty = false;
		bool seqLooping = false;
		bool seqNoFlip = false;
		bool dirFlip = false;
		bool playing = false;
		bool reverse = false;
		OptionalLite<bool> visibleUserOverride;
		OptionalLite<bool> visibleSyncOverride;
		bool applyPivot = true;
		bool applyMaterial = true;
		mutable bool hasUpdate = true;
		mutable bool someoneCheckingOnPlayId = false;

		Time curSeqTime;
		Time curFrameTime;

		int dirId = 0;
		int curFrameN = 0;
		int curLoopCount = 0;
		float playbackSpeed = 1.0f;

		Vector2f offsetPivot;

		AnimationPlayId curPlayId = 0;

		std::shared_ptr<const Animation> animation;
		const SpriteSheetEntry* spriteData = nullptr;

		const AnimationFrame* curFrame = nullptr;
		const AnimationSequence* curSeq = nullptr;
		const AnimationDirection* curDir = nullptr;

		std::shared_ptr<const Material> materialOverride;

		std::optional<String> nextSequence = {};
		
		ResourceObserver observer;
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
		void hash(const AnimationPlayer& player, Hash::Hasher& hasher);
	};
}

#include "graphics/sprite/animation.h"
#include "graphics/sprite/animation_player.h"
#include "graphics/sprite/sprite.h"
#include <gsl/gsl_assert>
#include "resources/resources.h"

using namespace Halley;

AnimationPlayer::AnimationPlayer(std::shared_ptr<const Animation> animation, const String& sequence, const String& direction)
{
	setAnimation(animation, sequence, direction);
}

AnimationPlayer& AnimationPlayer::playOnce(const String& sequence, const std::optional<String>& nextLoopingSequence)
{
	updateIfNeeded();

	curSeq = nullptr;
	setSequence(sequence);
	seqLooping = false;

	nextSequence = nextLoopingSequence;

	return *this;
}

AnimationPlayer& AnimationPlayer::setAnimation(std::shared_ptr<const Animation> v, const String& sequence, const String& direction)
{
	if (animation != v) {
		animation = v;
		if (animation) {
			observer.startObserving(*animation);
		} else {
			observer.stopObserving();
		}
		curDir = nullptr;
		curSeq = nullptr;
		dirId = -1;
	}

	updateIfNeeded();

	if (animation) {
		setSequence(sequence);
		setDirection(direction);
	}
	return *this;
}

AnimationPlayer& AnimationPlayer::setSequence(const String& sequence)
{
	curSeqName = sequence;
	nextSequence = {};
	updateIfNeeded();

	if (animation && (!curSeq || curSeq->getName() != sequence)) {
		curSeqTime = 0;
		curFrameTime = 0;
		curFrame = 0;
		curFrameLen = 0;
		curLoopCount = 0;
		curSeq = &animation->getSequence(sequence);

		seqLen = curSeq->numFrames();
		seqLooping = curSeq->isLooping();
		seqNoFlip = curSeq->isNoFlip();

		dirty = true;

		onSequenceStarted();
	}
	return *this;
}

AnimationPlayer& AnimationPlayer::setDirection(int direction)
{
	updateIfNeeded();

	if (animation && dirId != direction) {
		auto newDir = &animation->getDirection(direction);
		if (curDir != newDir) {
			curDir = newDir;
			dirFlip = curDir->shouldFlip();
			dirId = curDir->getId();
			dirty = true;
		}
	}
	return *this;
}

AnimationPlayer& AnimationPlayer::setDirection(const String& direction)
{
	curDirName = direction;
	updateIfNeeded();

	if (animation && (!curDir || curDir->getName() != direction)) {
		auto newDir = &animation->getDirection(direction);
		if (curDir != newDir) {
			curDir = newDir;
			dirFlip = curDir->shouldFlip();
			dirId = curDir->getId();
			dirty = true;
		}
	}
	return *this;
}

bool AnimationPlayer::trySetSequence(const String& sequence)
{
	updateIfNeeded();
	if (animation && animation->hasSequence(sequence)) {
		setSequence(sequence);
		return true;
	}
	return false;
}

AnimationPlayer& AnimationPlayer::setApplyPivot(bool apply)
{
	applyPivot = apply;
	return *this;
}

bool AnimationPlayer::isApplyingPivot() const
{
	return applyPivot;
}

void AnimationPlayer::update(Time time)
{
	updateIfNeeded();

	if (!animation) {
		return;
	}

	if (dirty) {
		resolveSprite();
		dirty = false;
	}

	const int prevFrame = curFrame;

	curSeqTime += time * playbackSpeed;
	curFrameTime += time * playbackSpeed;
		
	// Next frame time!
	if (curFrameTime >= curFrameLen) {
		for (int i = 0; i < 5 && curFrameTime >= curFrameLen; ++i) {
			curFrame++;
			curFrameTime -= curFrameLen;
			
			if (curFrame >= int(seqLen)) {
				if (seqLooping) {
					curFrame = 0;
					curSeqTime = curFrameTime;
					curLoopCount++;
				} else {
					curFrame = int(seqLen - 1);
					onSequenceDone();
				}
			}
		}
	}

	if (curFrame != prevFrame) {
		resolveSprite();
	}
}

void AnimationPlayer::updateSprite(Sprite& sprite) const
{
	if (animation && hasUpdate) {
		if (materialOverride) {
			sprite.setMaterial(materialOverride, true);
		} else {
			sprite.setMaterial(animation->getMaterial(), true);
		}
		
		sprite.setSprite(*spriteData, false);
		
		if (applyPivot && spriteData->pivot.isValid()) {
			auto sz = sprite.getSize();
			if (std::abs(sz.x) < 0.00001f) {
				sz.x = 1;
			}
			if (std::abs(sz.y) < 0.00001f) {
				sz.y = 1;
			}
			sprite.setPivot(spriteData->pivot + offsetPivot / sz);
		}
		sprite.setFlip(dirFlip && !seqNoFlip);
		if (visibleOverride) {
			sprite.setVisible(visibleOverride.value());
		}
		
		hasUpdate = false;
	}
}

AnimationPlayer& AnimationPlayer::setMaterialOverride(std::shared_ptr<Material> material)
{
	materialOverride = std::move(material);
	return *this;
}

std::shared_ptr<Material> AnimationPlayer::getMaterialOverride() const
{
	return materialOverride;
}

std::shared_ptr<const Material> AnimationPlayer::getMaterial() const
{
	return animation->getMaterial();
}

bool AnimationPlayer::isPlaying() const
{
	return playing;
}

String AnimationPlayer::getCurrentSequenceName() const
{
	return curSeq ? curSeq->getName() : "";
}

Time AnimationPlayer::getCurrentSequenceTime() const
{
	return curSeqTime;
}

int AnimationPlayer::getCurrentSequenceFrame() const
{
	return curFrame;
}

int AnimationPlayer::getCurrentSequenceLoopCount() const
{
	return curLoopCount;
}

String AnimationPlayer::getCurrentDirectionName() const
{
	return curDir ? curDir->getName() : "default";
}

AnimationPlayer& AnimationPlayer::setPlaybackSpeed(float value)
{
	playbackSpeed = value;
	return *this;
}

float AnimationPlayer::getPlaybackSpeed() const
{
	return playbackSpeed;
}

const Animation& AnimationPlayer::getAnimation() const
{
	return *animation;
}

bool AnimationPlayer::hasAnimation() const
{
	return static_cast<bool>(animation);
}

AnimationPlayer& AnimationPlayer::setOffsetPivot(Vector2f offset)
{
	offsetPivot = offset;
	hasUpdate = true;
	return *this;
}

void AnimationPlayer::syncWith(const AnimationPlayer& masterAnimator)
{
	setSequence(masterAnimator.getCurrentSequenceName());
	setDirection(masterAnimator.getCurrentDirectionName());
	visibleOverride = getCurrentSequenceName() == masterAnimator.getCurrentSequenceName();
	curFrame = clamp(masterAnimator.curFrame, 0, static_cast<int>(curSeq->numFrames()) - 1);
	curFrameTime = masterAnimator.curFrameTime;

	resolveSprite();
}

void AnimationPlayer::stepFrames(int amount)
{
	if (amount != 0) {
		curFrame = modulo(curFrame + amount, static_cast<int>(seqLen));
		curFrameTime = 0;

		resolveSprite();
	}
}

void AnimationPlayer::resolveSprite()
{
	updateIfNeeded();

	const auto& frame = curSeq->getFrame(curFrame);
	curFrameLen = std::max(1, frame.getDuration()) * 0.001; // 1ms minimum
	spriteData = &frame.getSprite(dirId);
	hasUpdate = true;
}

void AnimationPlayer::onSequenceStarted()
{
	playing = true;
}

void AnimationPlayer::onSequenceDone()
{
	if (nextSequence) {
		setSequence(nextSequence.value());
	} else {
		playing = false;
	}
}

void AnimationPlayer::updateIfNeeded()
{
	if (observer.needsUpdate()) {
		observer.update();
		dirId = -1;
		curDir = nullptr;
		curSeq = nullptr;
		setSequence(curSeqName);
		setDirection(curDirName);
	}
}

ConfigNode ConfigNodeSerializer<AnimationPlayer>::serialize(const AnimationPlayer& player, ConfigNodeSerializationContext& context)
{
	ConfigNode result = ConfigNode::MapType();
	result["animation"] = player.hasAnimation() ? player.getAnimation().getAssetId() : "";
	result["sequence"] = player.getCurrentSequenceName();
	result["direction"] = player.getCurrentDirectionName();
	result["applyPivot"] = player.isApplyingPivot();
	result["playbackSpeed"] = player.getPlaybackSpeed();
	return result;
}

AnimationPlayer ConfigNodeSerializer<AnimationPlayer>::deserialize(ConfigNodeSerializationContext& context, const ConfigNode& node)
{
	auto animName = node["animation"].asString("");
	auto anim = animName.isEmpty() ? std::shared_ptr<Animation>() : context.resources->get<Animation>(animName);

	auto player = AnimationPlayer(anim, node["sequence"].asString("default"), node["direction"].asString("default"));
	player.setApplyPivot(node["applyPivot"].asBool(true));
	player.setPlaybackSpeed(node["playbackSpeed"].asFloat(1.0f));	
	return player;
}

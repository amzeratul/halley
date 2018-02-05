#include "graphics/sprite/animation.h"
#include "graphics/sprite/animation_player.h"
#include "graphics/sprite/sprite.h"
#include <gsl/gsl_assert>

using namespace Halley;

AnimationPlayer::AnimationPlayer(std::shared_ptr<const Animation> animation, const String& sequence, const String& direction)
{
	setAnimation(animation, sequence, direction);
}

AnimationPlayer& AnimationPlayer::playOnce(const String& sequence)
{
	curSeq = nullptr;
	setSequence(sequence);
	seqLooping = false;
	return *this;
}

AnimationPlayer& AnimationPlayer::setAnimation(std::shared_ptr<const Animation> v, const String& sequence, const String& direction)
{
	if (animation != v) {
		animation = v;
		curDir = nullptr;
		curSeq = nullptr;
		dirId = -1;
	}

	if (animation) {
		setSequence(sequence);
		setDirection(direction);
	}
	return *this;
}

AnimationPlayer& AnimationPlayer::setSequence(const String& sequence)
{
	if (animation && (!curSeq || curSeq->getName() != sequence)) {
		curSeqTime = 0;
		curFrameTime = 0;
		curFrame = -1;
		curFrameLen = 0;
		curSeq = &animation->getSequence(sequence);
		Expects(curSeq);

		seqLen = curSeq->numFrames();
		seqLooping = curSeq->isLooping();
		seqNoFlip = curSeq->isNoFlip();

		dirty = true;
		update(0.0001);

		onSequenceStarted();
	}
	return *this;
}

AnimationPlayer& AnimationPlayer::setDirection(int direction)
{
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

void AnimationPlayer::update(Time time)
{
	if (animation) {
		const int prevFrame = curFrame;

		curSeqTime += time * playbackSpeed;
		curFrameTime += time * playbackSpeed;
		while (curFrameTime >= curFrameLen) {
			curFrame++;
			curFrameTime -= curFrameLen;
			
			if (curFrame >= int(seqLen)) {
				if (seqLooping) {
					curFrame = 0;
					curSeqTime = curFrameTime;
				} else {
					onSequenceDone();
				}
			}
		}

		dirty |= curFrame != prevFrame;
		if (dirty) {
			resolveSprite();
			dirty = false;
		}
	}
}

void AnimationPlayer::updateSprite(Sprite& sprite) const
{
	if (animation && hasUpdate) {
		if (materialOverride) {
			sprite.setMaterial(materialOverride);
		} else {
			sprite.setMaterial(animation->getMaterial());
		}
		sprite.setSprite(*spriteData, false);
		if (applyPivot) {
			sprite.setPivot(spriteData->pivot + offsetPivot / sprite.getSize());
		}
		sprite.setFlip(dirFlip && !seqNoFlip);
		hasUpdate = false;
	}
}

AnimationPlayer& AnimationPlayer::setMaterialOverride(std::shared_ptr<Material> material)
{
	materialOverride = material;
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

void AnimationPlayer::setOffsetPivot(Vector2f offset)
{
	offsetPivot = offset;
	hasUpdate = true;
}

void AnimationPlayer::resolveSprite()
{
	Expects(curSeq);
	auto& frame = curSeq->getFrame(curFrame);
	curFrameLen = frame.getDuration() * 0.001;
	spriteData = &frame.getSprite(dirId);
	hasUpdate = true;
}

void AnimationPlayer::onSequenceStarted()
{
	playing = true;
}

void AnimationPlayer::onSequenceDone()
{
	playing = false;
}

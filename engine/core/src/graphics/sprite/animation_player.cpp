#include "graphics/sprite/animation.h"
#include "graphics/sprite/animation_player.h"
#include "graphics/sprite/sprite.h"
#include <gsl/gsl_assert>

using namespace Halley;

AnimationPlayer::AnimationPlayer(std::shared_ptr<const Animation> animation, String sequence, String direction)
{
	setAnimation(animation, sequence, direction);
}

AnimationPlayer& AnimationPlayer::playOnce(String sequence)
{
	curSeq = nullptr;
	setSequence(sequence);
	seqLooping = false;
	return *this;
}

AnimationPlayer& AnimationPlayer::setAnimation(std::shared_ptr<const Animation> v, String sequence, String direction)
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

AnimationPlayer& AnimationPlayer::setSequence(String sequence)
{
	if (!curSeq || curSeq->getName() != sequence) {
		Expects(animation);
		curTime = 0;
		curFrame = 0;
		curSeq = &animation->getSequence(sequence);

		seqFPS = curSeq->getFPS();
		seqLen = curSeq->numFrames();
		seqLooping = curSeq->isLooping();
		seqTimeLen = seqLen / seqFPS;
		seqNoFlip = curSeq->isNoFlip();

		dirty = true;
		onSequenceStarted();
	}
	return *this;
}

AnimationPlayer& AnimationPlayer::setDirection(int direction)
{
	if (dirId != direction) {
		Expects(animation);
		
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

AnimationPlayer& AnimationPlayer::setDirection(String direction)
{
	if (!curDir || curDir->getName() != direction) {
		Expects(animation);

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

AnimationPlayer& AnimationPlayer::setApplyPivot(bool apply)
{
	applyPivot = apply;
	return *this;
}

void AnimationPlayer::update(Time time)
{
	if (animation) {
		int prevFrame = curFrame;
		curFrame = std::min(int(curTime * seqFPS), int(seqLen - 1));

		dirty |= curFrame != prevFrame;
		if (dirty) {
			resolveSprite();
			dirty = false;
		}
		
		curTime += time * playbackSpeed;

		if (curTime > seqTimeLen) {
			// Reached end of sequence			
			if (seqLooping) {
				curTime = ::fmod(curTime, seqTimeLen);
			} else {
				curTime = seqTimeLen;
				onSequenceDone();
			}
		}
	}
}

void AnimationPlayer::updateSprite(Sprite& sprite) const
{
	if (hasUpdate) {
		if (materialOverride) {
			sprite.setMaterial(materialOverride);
		} else {
			sprite.setMaterial(animation->getMaterial());
		}
		sprite.setSprite(*spriteData, applyPivot);
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

const String& AnimationPlayer::getCurrentSequenceName() const
{
	return curSeq->getName();
}

Time AnimationPlayer::getCurrentSequenceTime() const
{
	return curTime;
}

int AnimationPlayer::getCurrentSequenceFrame() const
{
	return curFrame;
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

void AnimationPlayer::resolveSprite()
{
	spriteData = &curSeq->getFrame(curFrame).getSprite(dirId);
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

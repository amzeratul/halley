#include "graphics/sprite/animation.h"
#include "graphics/sprite/animation_player.h"
#include "graphics/sprite/sprite.h"

using namespace Halley;

AnimationPlayer::AnimationPlayer(std::shared_ptr<Animation> animation, String sequence, String direction)
{
	setAnimation(animation, sequence, direction);
}

void AnimationPlayer::setAnimation(std::shared_ptr<Animation> v, String sequence, String direction)
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
}

void AnimationPlayer::setSequence(String sequence)
{
	if (!curSeq || curSeq->getName() != sequence) {
		assert(animation);
		curTime = 0;
		curFrame = 0;
		curSeq = &animation->getSequence(sequence);

		seqFPS = curSeq->getFPS();
		seqLen = curSeq->numFrames();
		seqLooping = curSeq->isLooping();
		seqTimeLen = seqLen / seqFPS;
		seqNoFlip = curSeq->isNoFlip();

		dirty = true;
	}
}

void AnimationPlayer::setDirection(int direction)
{
	if (dirId != direction) {
		assert(animation);
		
		auto newDir = &animation->getDirection(direction);
		if (curDir != newDir) {
			curDir = newDir;
			dirFlip = curDir->shouldFlip();
			dirId = curDir->getId();
			dirty = true;
		}
	}
}

void AnimationPlayer::setDirection(String direction)
{
	if (!curDir || curDir->getName() != direction) {
		assert(animation);

		auto newDir = &animation->getDirection(direction);
		if (curDir != newDir) {
			curDir = newDir;
			dirFlip = curDir->shouldFlip();
			dirId = curDir->getId();
			dirty = true;
		}
	}
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
		
		curTime += time;

		if (curTime > seqTimeLen) {
			// Reached end of sequence			
			if (seqLooping) {
				curTime = ::fmod(curTime, seqTimeLen);
			} else {
				curTime = seqTimeLen;
			}
		}
	}
}

void AnimationPlayer::updateSprite(Sprite& sprite) const
{
	if (hasUpdate) {
		sprite.setMaterial(animation->getMaterial());
		sprite.setSprite(*spriteData);
		sprite.setFlip(dirFlip && !seqNoFlip);
		hasUpdate = false;
	}
}

void AnimationPlayer::resolveSprite()
{
	spriteData = &curSeq->getFrame(curFrame).getSprite(dirId);
	hasUpdate = true;
}

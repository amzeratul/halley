#include "animation.h"
#include "animation_player.h"
#include "sprite.h"

using namespace Halley;

void AnimationPlayer::setAnimation(std::shared_ptr<Animation> v)
{
	animation = v;
	setSequence("default");
	setDirection("default");
}

void AnimationPlayer::setSequence(String sequence)
{
	assert(animation);
	curTime = 0;
	curFrame = 0;
	curSeq = &animation->getSequence(sequence);
	
	seqFPS = curSeq->getFPS();
	seqLen = curSeq->numFrames();

	dirty = true;
}

void AnimationPlayer::setDirection(String direction)
{
	assert(animation);
	curDir = &animation->getDirection(direction);
	dirty = true;
}

void AnimationPlayer::update(Time time)
{
	if (animation) {
		int prevFrame = curFrame;
		curFrame = int(curTime * seqFPS) % seqLen;
		dirty |= curFrame != prevFrame;
		if (dirty) {
			resolveSprite();
			dirty = false;
		}
		curTime += time;
	}
}

void AnimationPlayer::updateSprite(Sprite& sprite) const
{
	if (animation) {
		sprite.setMaterial(animation->getMaterial());
		sprite.setSprite(*spriteData);
	}
}

void AnimationPlayer::resolveSprite()
{
	spriteData = &curSeq->getFrame(curFrame).getSprite(curDir->getId());
}

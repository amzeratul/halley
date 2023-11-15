#include "halley/graphics/sprite/animation.h"
#include "halley/graphics/sprite/animation_player.h"
#include "halley/graphics/sprite/sprite.h"
#include <gsl/gsl_assert>

#include "halley/graphics/material/material.h"
#include "halley/resources/resources.h"

using namespace Halley;

AnimationPlayer::AnimationPlayer(std::shared_ptr<const Animation> animation, const String& sequence, const String& direction)
{
	setAnimation(animation, sequence, direction);
}

AnimationPlayer& AnimationPlayer::playOnce(const String& sequence, const std::optional<String>& nextLoopingSequence, bool reverse)
{
	updateResourceIfNeeded();

	curSeq = nullptr;
	setSequence(sequence);
	seqLooping = false;

	nextSequence = nextLoopingSequence;

	this->reverse = reverse;
	if (reverse) {
		curFrameN = static_cast<int>(curSeq->numFrames() - 1);
	}

	return *this;
}

AnimationPlayer& AnimationPlayer::stop()
{
	curSeq = nullptr;
	playing = false;

	updateResourceIfNeeded();
	
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
		curFrame = nullptr;
		dirId = -1;
	}

	updateResourceIfNeeded();

	if (animation) {
		setSequence(sequence);
		setDirection(direction);
	}
	return *this;
}

AnimationPlayer& AnimationPlayer::setSequence(const String& _sequence)
{
	curSeqName = _sequence; // DO NOT use _sequence after this, it can be a reference to nextSequence, which is changed on the next line
	nextSequence = {};
	updateResourceIfNeeded();

	if (animation && (!curSeq || curSeq->getName() != curSeqName)) {
		curSeqTime = 0;
		curFrameTime = 0;
		curFrameN = 0;
		curFrame = nullptr;
		curLoopCount = 0;
		curSeq = &animation->getSequence(curSeqName);

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
	updateResourceIfNeeded();

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
	updateResourceIfNeeded();

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
	updateResourceIfNeeded();
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
	updateResourceIfNeeded();

	if (!animation) {
		return;
	}

	if (dirty) {
		resolveSprite();
		dirty = false;
	}

	const int prevFrame = curFrameN;

	curSeqTime += time * (reverse ? -playbackSpeed : playbackSpeed);
	curFrameTime += time * (reverse ? -playbackSpeed : playbackSpeed);

	// Prev frame time!
	if (curFrameTime < 0) {
		for (int i = 0; i < 5 && curFrameTime < 0; ++i) {
			--curFrameN;
			const int actualFrame = curFrameN >= 0 ? curFrameN : (seqLooping ? static_cast<int>(seqLen) - 1 : 0);
			curFrameTime += std::max(curSeq->getFrame(actualFrame).getDuration(), 1) * 0.001;
			
			if (curFrameN < 0) {
				curFrameN = actualFrame;
				if (seqLooping) {
					curSeqTime = curFrameTime;
					for (int i = 0; i < curFrameN; ++i) {
						// Add the time of previous frames
						curSeqTime += std::max(curSeq->getFrame(i).getDuration(), 1) * 0.001;
					}
					curLoopCount++;
				} else {
					onSequenceDone();
				}
			}
		}
	}

	// Next frame time!
	else {
		const auto curFrameLen = std::max(curFrame ? curFrame->getDuration() : 100, 1) * 0.001;
		if (curFrameTime >= curFrameLen) {
			for (int i = 0; i < 5 && curFrameTime >= curFrameLen; ++i) {
				curFrameN++;
				curFrameTime -= curFrameLen;

				if (curFrameN >= int(seqLen)) {
					if (seqLooping) {
						curFrameN = 0;
						curSeqTime = curFrameTime;
						curLoopCount++;
					}
					else {
						curFrameN = int(seqLen - 1);
						onSequenceDone();
					}
				}
			}
		}
	}

	if (curFrameN != prevFrame) {
		resolveSprite();
	}
}

void AnimationPlayer::updateSprite(Sprite& sprite) const
{
	if (animation && spriteData && (hasUpdate || !sprite.hasMaterial())) {
		if (applyMaterial || !sprite.hasMaterial()) {
			const auto& newMaterial = materialOverride ? materialOverride : animation->getMaterial();
			if (!sprite.hasCompatibleMaterial(*newMaterial)) {
				sprite.setMaterial(newMaterial);
			}
		} else {
			sprite.getMutableMaterial().set(0, animation->getSpriteSheet().getTexture());
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
		sprite.setFlip(isFlipped());
		if (visibleOverride) {
			sprite.setVisible(visibleOverride.value());
		}
		
		hasUpdate = false;
	}
}

AnimationPlayer& AnimationPlayer::setMaterialOverride(std::shared_ptr<const Material> material)
{
	materialOverride = std::move(material);
	return *this;
}

std::shared_ptr<const Material> AnimationPlayer::getMaterialOverride() const
{
	return materialOverride;
}

std::shared_ptr<const Material> AnimationPlayer::getMaterial() const
{
	return animation->getMaterial();
}

void AnimationPlayer::setApplyMaterial(bool apply)
{
	applyMaterial = apply;
}

void AnimationPlayer::setReversePlaying(bool reverse)
{
	this->reverse = reverse;
}

bool AnimationPlayer::isApplyingMaterial() const
{
	return applyMaterial;
}

bool AnimationPlayer::isPlaying() const
{
	return playing;
}

const String& AnimationPlayer::getCurrentSequenceName() const
{
	updateResourceIfNeeded();
	return curSeq ? curSeq->getName() : curSeqName;
}

Time AnimationPlayer::getCurrentSequenceTime() const
{
	return curSeqTime;
}

int AnimationPlayer::getCurrentSequenceFrame() const
{
	return curFrameN;
}

Time AnimationPlayer::getCurrentSequenceFrameTime() const
{
	return curFrameTime;
}

int AnimationPlayer::getCurrentSequenceLoopCount() const
{
	return curLoopCount;
}

int AnimationPlayer::getCurrentSequenceLength() const
{
	return static_cast<int>(seqLen);
}

String AnimationPlayer::getCurrentDirectionName() const
{
	updateResourceIfNeeded();
	return curDir ? curDir->getName() : "default";
}

int AnimationPlayer::getCurrentDirectionId() const
{
	return dirId;
}

bool AnimationPlayer::isFlipped() const
{
	return dirFlip && !seqNoFlip;
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

std::shared_ptr<const Animation> AnimationPlayer::getAnimationPtr() const
{
	return animation;
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

void AnimationPlayer::syncWith(const AnimationPlayer& masterAnimator, bool hideIfNotSynchronized)
{
	updateResourceIfNeeded();
	setState(masterAnimator.getCurrentSequenceName(), masterAnimator.getCurrentDirectionName(), masterAnimator.curFrameN, masterAnimator.curFrameTime, hideIfNotSynchronized);
}

void AnimationPlayer::setState(const String& sequenceName, const String& directionName, int currentFrame, Time currentFrameTime, bool hideIfNotSynchronized)
{
	setSequence(sequenceName);
	setDirection(directionName);

	const auto oldVisibleOverride = visibleOverride;
	const auto oldCurFrame = curFrameN;
	
	visibleOverride = !hideIfNotSynchronized || getCurrentSequenceName() == sequenceName;
	curFrameN = clamp(currentFrame, 0, curSeq ? static_cast<int>(curSeq->numFrames()) - 1 : 0);
	curFrameTime = currentFrameTime;

	if (dirty || oldVisibleOverride != visibleOverride || oldCurFrame != curFrameN) {
		dirty = false;
		resolveSprite();
	}
}

void AnimationPlayer::setTiming(int currentFrame, Time currentFrameTime)
{
	updateResourceIfNeeded();
	curFrameN = clamp(currentFrame, 0, curSeq ? static_cast<int>(curSeq->numFrames()) - 1 : 0);
	curFrameTime = currentFrameTime;
	dirty = false;
	resolveSprite();
}


void AnimationPlayer::stepFrames(int amount)
{
	if (amount != 0) {
		curFrameN = modulo(curFrameN + amount, static_cast<int>(seqLen));
		curFrameTime = 0;

		resolveSprite();
	}
}

std::optional<Vector2i> AnimationPlayer::getCurrentActionPoint(const String& actionPointId) const
{
	updateResourceIfNeeded();
	if (animation && curSeq && curDir) {
		auto point = animation->getActionPoint(actionPointId, curSeq->getId(), curDir->getId(), curFrameN);
		if (point && isFlipped()) {
			point->x *= -1;
		}
		return point;
	}
	return {};
}

void AnimationPlayer::resolveSprite()
{
	updateResourceIfNeeded();

	if (curSeq && curSeq->numFrames() > 0) {
		curFrame = &curSeq->getFrame(curFrameN);
		spriteData = &(curFrame->getSprite(dirId));
		hasUpdate = true;
	}
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

void AnimationPlayer::updateResourceIfNeeded() const
{
#ifdef ENABLE_HOT_RELOAD
	if (observer.needsUpdate()) {
		const_cast<AnimationPlayer*>(this)->doUpdateResource();
	}
#endif
}

void AnimationPlayer::doUpdateResource()
{
	observer.update();
	dirId = -1;
	curDir = nullptr;
	curSeq = nullptr;
	setSequence(curSeqName);
	setDirection(curDirName);
}

AnimationPlayerLite::AnimationPlayerLite(std::shared_ptr<const Animation> animation, const String& sequence, const String& direction)
{
	setAnimation(std::move(animation), sequence, direction);
}

AnimationPlayerLite& AnimationPlayerLite::setAnimation(std::shared_ptr<const Animation> animation, const String& sequence, const String& direction)
{
	this->animation = std::move(animation);
	setSequence(sequence);
	setDirection(direction);
	return *this;
}

AnimationPlayerLite& AnimationPlayerLite::setSequence(const String& sequence)
{
	curSeqIdx = static_cast<int>(animation->getSequenceIdx(sequence));
	curFrame = -1;
	return *this;
}

AnimationPlayerLite& AnimationPlayerLite::setDirection(int direction)
{
	curDir = direction;
	curFrame = -1;
	return *this;
}

AnimationPlayerLite& AnimationPlayerLite::setDirection(const String& direction)
{
	setDirection(animation->getDirection(direction).getId());
	return *this;
}

void AnimationPlayerLite::update(Time time, Sprite& sprite)
{
	bool changed = false;
	curTime += static_cast<float>(time);

	const auto& seq = animation->getSequence(curSeqIdx.value_or(0));

	if (curFrame == -1) {
		curFrame = 0;
		curTime = 0;
		changed = true;
	} else {
		while (true) {
			const float duration = seq.getFrame(curFrame).getDuration() * 0.001f;
			if (curTime > duration) {
				curTime -= duration;
				curFrame = modulo(curFrame + 1, static_cast<int>(seq.numFrames()));
				changed = true;
			} else {
				break;
			}
		}
	}

	if (changed) {
		sprite
			.setMaterial(animation->getMaterial())
			.setSprite(seq.getFrame(curFrame).getSprite(curDir));
	}
}

ConfigNode ConfigNodeSerializer<AnimationPlayer>::serialize(const AnimationPlayer& player, const EntitySerializationContext& context)
{
	ConfigNode result = ConfigNode::MapType();
	result["animation"] = player.hasAnimation() ? player.getAnimation().getAssetId() : "";
	result["sequence"] = player.getCurrentSequenceName();
	result["direction"] = player.getCurrentDirectionName();
	if (player.isApplyingPivot() != true) {
		result["applyPivot"] = player.isApplyingPivot();
	}
	if (player.getPlaybackSpeed() != 1.0f) {
		result["playbackSpeed"] = player.getPlaybackSpeed();
	}
	if (player.isApplyingMaterial() != true) {
		result["applyMaterial"] = player.isApplyingMaterial();
	}
	return result;
}

AnimationPlayer ConfigNodeSerializer<AnimationPlayer>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Undefined) {
		return AnimationPlayer();
	}
	
	auto animName = node["animation"].asString("");
	auto anim = animName.isEmpty() ? std::shared_ptr<Animation>() : context.resources->get<Animation>(animName);

	auto player = AnimationPlayer(anim, node["sequence"].asString("default"), node["direction"].asString("default"));
	player.setApplyPivot(node["applyPivot"].asBool(true));
	player.setPlaybackSpeed(node["playbackSpeed"].asFloat(1.0f));
	player.setApplyMaterial(node["applyMaterial"].asBool(true));
	return player;
}

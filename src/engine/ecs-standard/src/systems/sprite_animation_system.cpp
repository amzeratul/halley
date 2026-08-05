#include <systems/sprite_animation_system.h>

using namespace Halley;

class SpriteAnimationSystem final : public SpriteAnimationSystemBase<SpriteAnimationSystem>, ISpriteAnimationSystemInterface {
public:
	void init()
	{
		getWorld().setInterface(static_cast<ISpriteAnimationSystemInterface*>(this));
	}

	void update(Time time) {
		const auto viewPort = getViewPort();
		updateAnimators(time, viewPort);
		updateReplicators(viewPort);
	}

	void onMessageReceived(const PlayAnimationMessage& msg, MainFamily& e) override
	{
		e.spriteAnimation.player.setSequence(msg.sequence);
	}

	void onMessageReceived(const PlayAnimationOnceMessage& msg, MainFamily& e) override
	{
		e.spriteAnimation.player.playOnce(msg.sequence);
	}

	void onMessageReceived(const PlayNetworkAnimationSystemMessage& msg) override
	{
		if (const auto e = mainFamily.tryFind(msg.entity); e != nullptr) {
			if (msg.animation) {
				e->spriteAnimation.player.setAnimation(getResources().get<Animation>(*msg.animation));	
			}

			e->spriteAnimation.player.setDirection(msg.direction);
			
			if (msg.once) {
				e->spriteAnimation.player.playOnce(msg.sequence, {}, msg.reverse);
			} else {
				e->spriteAnimation.player.setSequence(msg.sequence);
			}
		}
	}

	void onEntitiesAdded(Span<MainFamily> es)
	{
		const auto viewPort = getViewPort();
		for (auto& e: es) {
			e.spriteAnimation.player.update(0.0f);
			updateSprite(e, viewPort, false);
		}
	}

	void onEntitiesReloaded(Span<MainFamily*> es)
	{
		const auto viewPort = getViewPort();
		for (auto& e: es) {
			e->spriteAnimation.player.update(0.0f);
			updateSprite(*e, viewPort, false);
		}
	}

	void setCallback(Callback callback) override
	{
		this->callback = std::move(callback);
	}

private:
	Callback callback;

	Rect4f getViewPort() const
	{
		return getScreenService().getCameraViewPort().grow(10, 10, 10, 10);
	}

	void updateAnimators(Time time, Rect4f viewPort)
	{
		for (auto& e : mainFamily) {
			if (e.spriteAnimation.player.isActiveAnimation()) {
				if (!isCulledByFixedBounds(e.transform2D, e.spriteAnimation, viewPort)) {
					e.spriteAnimation.player.update(time);
					updateSprite(e, viewPort, false);
				}
				updateEvents(e);
			}
		}
	}

	bool isCulledByFixedBounds(const Transform2DComponent& transform2D, const SpriteAnimationComponent& spriteAnimation, const Rect4f& viewPort) const
	{
		if (spriteAnimation.cullBounds) {
			return !viewPort.overlaps(transform2D.getGlobalPositionWithHeight() + *spriteAnimation.cullBounds);
		}
		return false;
	}

	Rect4f getAnimationBounds(const Transform2DComponent& transform2D, const SpriteAnimationComponent& spriteAnimation) const
	{
		auto& player = spriteAnimation.player;
		const auto nextBounds = Rect4f(player.getAnimation().getBounds());
		if (player.getAnimation().getMaterial()->getDefinition().hasTagIdx(MaterialTags::NoRotate)) {
			return Rect4f(transform2D.transformPointWithHeightNoRotate(nextBounds.getTopLeft()), transform2D.transformPointWithHeightNoRotate(nextBounds.getBottomRight()));
		} else {
			return Rect4f(transform2D.transformPointWithHeight(nextBounds.getTopLeft()), transform2D.transformPointWithHeight(nextBounds.getBottomRight()));
		}
	}

	bool isInBounds(const Transform2DComponent& transform2D, const SpriteComponent& sprite, const SpriteAnimationComponent& spriteAnimation, const Rect4f& viewPort) const
	{
		return sprite.sprite.getAABB().overlaps(viewPort) || getAnimationBounds(transform2D, spriteAnimation).overlaps(viewPort);
	}

	bool isInBoundsWithCull(const Transform2DComponent& transform2D, const SpriteComponent& sprite, const SpriteAnimationComponent& spriteAnimation, const Rect4f& viewPort) const
	{
		return !isCulledByFixedBounds(transform2D, spriteAnimation, viewPort) && isInBounds(transform2D, sprite, spriteAnimation, viewPort);
	}

	void updateSprite(MainFamily& e, Rect4f viewPort, bool ignoreBounds)
	{
		auto& player = e.spriteAnimation.player;
		if (e.spriteAnimation.updateSprite && player.hasAnimation()) {
			if (ignoreBounds || isInBounds(e.transform2D, e.sprite, e.spriteAnimation, viewPort)) {
				player.updateSprite(e.sprite.sprite);
			}
		}
	}

	void updateEvents(MainFamily& e)
	{
		auto& player = e.spriteAnimation.player;
		if (e.spriteAnimationEvents && player.hasAnimation()) {
			auto& events = *e.spriteAnimationEvents;

			const auto animIdx = player.getAnimation().getAssetIdx();
			const auto seqId = player.getCurrentSequenceId();
			const auto seqDir = player.getCurrentDirectionId();
			const auto seqFrame = player.getCurrentSequenceFrame();

			SpriteAnimationEvent event;

			if (animIdx != events.prevAnimIdx) {
				events.prevAnimIdx = animIdx;
				event.animationChanged = true;
			}
			if (seqId != events.prevSeqId) {
				events.prevSeqId = seqId;
				event.sequenceChanged = true;
			}
			if (seqDir != events.prevDir) {
				events.prevDir = seqDir;
				event.directionChanged = true;
			}
			if (seqFrame != events.prevFrame) {
				events.prevFrame = seqFrame;
				event.frameChanged = true;
			}

			if ((event.animationChanged || event.directionChanged || event.frameChanged || event.sequenceChanged) && callback) {

				event.entityId = e.entityId;
				event.tags = events.tags.const_span();
				
				event.animation = player.getAnimationPtr().get();
				event.sequenceName = player.getCurrentSequenceName();
				event.direction = seqDir;
				event.frame = seqFrame;
				
				callback(event);
			}
		}
	}

	Vector<Vector<ReplicatorFamily*>> replicatorsPerLevel;

	void updateReplicators(Rect4f viewPort)
	{
		replicatorsPerLevel.resize(std::max<size_t>(replicatorsPerLevel.size(), 8));
		for (auto& l: replicatorsPerLevel) {
			l.clear();
		}

		for (auto& e : replicatorFamily) {
			if (true || isInBoundsWithCull(e.transform2D, e.sprite, e.spriteAnimation, viewPort)) { // can't cull as sprite replicating might be in view when this isn't
				auto depth = e.transform2D.getDepth();
				if (replicatorsPerLevel.size() <= depth) {
					replicatorsPerLevel.resize(nextPowerOf2(depth + 1));
				}
				replicatorsPerLevel[depth].push_back(&e);
			}
		}

		for (const auto& level: replicatorsPerLevel) {
			for (const auto& replicator: level) {
				auto e = getWorld().getEntity(replicator->entityId);
				if (auto parent = e.tryGetParent()) {
					if (const auto* parentAnimation = parent->tryGetComponent<SpriteAnimationComponent>()) {
						replicator->spriteAnimation.player.syncWith(parentAnimation->player, false);

						if (replicator->spriteAnimation.updateSprite && replicator->spriteAnimation.player.hasAnimation() && isInBoundsWithCull(replicator->transform2D, replicator->sprite, replicator->spriteAnimation, viewPort)) {
							replicator->spriteAnimation.player.updateSprite(replicator->sprite.sprite);
						}
					}
				}
			}
		}
	}
};

REGISTER_SYSTEM(SpriteAnimationSystem)


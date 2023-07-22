#include <systems/sprite_animation_system.h>

using namespace Halley;

class SpriteAnimationSystem final : public SpriteAnimationSystemBase<SpriteAnimationSystem> {
public:
	void update(Time time) {
		updateAnimators(time);
		updateReplicators();
	}

	void onMessageReceived(const PlayAnimationMessage& msg, MainFamily& e)
	{
		e.spriteAnimation.player.setSequence(msg.sequence);
	}

	void onMessageReceived(const PlayAnimationOnceMessage& msg, MainFamily& e)
	{
		e.spriteAnimation.player.playOnce(msg.sequence);
	}

	void onEntitiesAdded(Span<MainFamily> es)
	{
		for (auto& e: es) {
			e.spriteAnimation.player.update(0.0f);
			e.spriteAnimation.player.updateSprite(e.sprite.sprite);
		}
	}

	void onEntitiesReloaded(Span<MainFamily*> es)
	{
		for (auto& e : es) {
			e->spriteAnimation.player.update(0.0f);
			e->spriteAnimation.player.updateSprite(e->sprite.sprite);
		}
	}

private:

	void updateAnimators(Time time)
	{
		const auto viewPort = getScreenService().getCameraViewPort().grow(10, 10, 10, 10);
		for (auto& e : mainFamily) {
			auto& sprite = e.sprite.sprite;

			auto& player = e.spriteAnimation.player;
			player.update(time);

			if (player.hasAnimation()) {
				auto spriteBounds = Rect4f(player.getAnimation().getBounds()) + e.transform2D.getGlobalPosition();
				if (spriteBounds.overlaps(viewPort)) {
					player.updateSprite(sprite);
				}
			}
		}
	}

	void updateReplicators()
	{
		for (auto& e : replicatorFamily) {
			auto entity = getWorld().getEntity(e.entityId);
			auto parent = entity.tryGetParent();
			if (parent) {
				const auto parentAnimation = parent.value().tryGetComponent<SpriteAnimationComponent>();
				if (parentAnimation) {
					e.spriteAnimation.player.syncWith(parentAnimation->player, false);
					e.spriteAnimation.player.updateSprite(e.sprite.sprite);
				}
			}
		}
	}
};

REGISTER_SYSTEM(SpriteAnimationSystem)


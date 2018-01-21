#include "systems/sprite_animation_system.h"

class SpriteAnimationSystem final : public SpriteAnimationSystemBase<SpriteAnimationSystem> {
public:
	void update(Halley::Time time, MainFamily& e)
	{
		auto& sprite = e.sprite.sprite;

		auto vel = e.velocity.velocity;
		int dir = std::abs(vel.y) > std::abs(vel.x) ? (vel.y < 0 ? 0 : 2) : (vel.x < 0 ? 3 : 1);

		auto& player = e.spriteAnimation.player;
		player.setDirection(dir);
		player.update(time);
		player.updateSprite(sprite);
	}

	void onEntitiesAdded(Halley::Span<MainFamily> es)
	{
		for (auto& e: es) {
			e.spriteAnimation.player.update(0);
			e.spriteAnimation.player.updateSprite(e.sprite.sprite);
		}
	}
};

REGISTER_SYSTEM(SpriteAnimationSystem)

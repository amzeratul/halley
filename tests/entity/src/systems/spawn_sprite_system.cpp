#include "systems/spawn_sprite_system.h"

#include "components/position_component.h"
#include "components/sprite_animation_component.h"
#include "components/sprite_component.h"
#include "components/time_component.h"
#include "components/velocity_component.h"

using namespace Halley;

class SpawnSpriteSystem final : public SpawnSpriteSystemBase<SpawnSpriteSystem> {
public:
	void update(Time)
	{
		auto& resources = getAPI().core->getResources();
		const int targetEntities = Debug::isDebug() ? 200 : 10000;
		const int nToSpawn = std::min(targetEntities - int(getWorld().numEntities()), std::max(1, targetEntities / 60));
		auto anim = resources.get<Animation>("ella");
		for (int i = 0; i < nToSpawn; i++) {
			auto& r = Random::getGlobal();

			getWorld().createEntity()
				.addComponent(PositionComponent(Vector2f(r.getFloat(0.0f, 1280.0f), r.getFloat(0.0f, 720.0f))))
				.addComponent(VelocityComponent(Vector2f(r.getFloat(200.0f, 300.0f), 0.0f).rotate(Angle1f::fromDegrees(r.getFloat(0.0f, 360.0f)))))
				.addComponent(SpriteComponent(Sprite(), 0))
				.addComponent(TimeComponent(r.getFloat(0.0f, 2.0f)))
				.addComponent(SpriteAnimationComponent(AnimationPlayer(anim, "run")));
		}
	}
};

REGISTER_SYSTEM(SpawnSpriteSystem)

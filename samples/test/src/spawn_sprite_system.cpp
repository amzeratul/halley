#include "../gen/cpp/systems/spawn_sprite_system.h"

#include "../gen/cpp/components/position_component.h"
#include "../gen/cpp/components/sprite_animation_component.h"
#include "../gen/cpp/components/sprite_component.h"
#include "../gen/cpp/components/time_component.h"
#include "../gen/cpp/components/velocity_component.h"

using namespace Halley;

void SpawnSpriteSystem::update(Halley::Time)
{
	auto& world = getWorld();
	auto& resources = getAPI().core->getResources();
	const int targetEntities = Halley::Debug::isDebug() ? 20 : 10000;
	const int nToSpawn = std::min(targetEntities - int(world.numEntities()), std::max(1, targetEntities / 60));
	for (int i = 0; i < nToSpawn; i++) {
		auto& r = Random::getGlobal();

		world.createEntity()
			.addComponent(new PositionComponent(Vector2f(r.getFloat(0.0f, 1280.0f), r.getFloat(0.0f, 720.0f))))
			.addComponent(new VelocityComponent(Vector2f(r.getFloat(200.0f, 300.0f), 0.0f).rotate(Angle1f::fromDegrees(r.getFloat(0.0f, 360.0f)))))
			.addComponent(new SpriteComponent())
			.addComponent(new TimeComponent(r.getFloat(0.0f, 1.0f)))
			.addComponent(new SpriteAnimationComponent(AnimationPlayer(resources.get<Animation>("ella.yaml"), "run")));
	}
}

REGISTER_SYSTEM(SpawnSpriteSystem)

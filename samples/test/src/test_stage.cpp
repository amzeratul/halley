#include "test_stage.h"
#include <gen/cpp/systems/movement_system.h>
#include <gen/cpp/systems/sprite_animation_system.h>
#include <gen/cpp/systems/time_system.h>
#include <gen/cpp/systems/render_system.h>

using namespace Halley;

void TestStage::init()
{
	world = std::make_unique<Halley::World>(&getAPI());
	world->addSystem(std::make_unique<TimeSystem>(), TimeLine::FixedUpdate);
	world->addSystem(std::make_unique<MovementSystem>(), TimeLine::FixedUpdate);
	world->addSystem(std::make_unique<SpriteAnimationSystem>(), TimeLine::FixedUpdate);
	world->addSystem(std::make_unique<RenderSystem>(), TimeLine::Render);

	for (int i = 0; i < 100; i++) {
		auto& r = Random::getGlobal();

		auto posComp = new PositionComponent();
		posComp->position = Vector2f(r.getFloat(0.0f, 1280.0f), r.getFloat(0.0f, 720.0f));

		auto velComp = new VelocityComponent();
		velComp->velocity = Vector2f(r.getFloat(200.0f, 300.0f), 0.0f).rotate(Angle1f::fromDegrees(r.getFloat(0.0f, 360.0f)));

		auto spriteComp = new SpriteComponent();
		auto& sprite = spriteComp->sprite;
		auto spriteSheet = getResource<SpriteSheet>("sprites/ella.json");
		auto material = getResource<Material>("shaders/sprite.yaml");
		(*material)["tex0"] = spriteSheet->getTexture();
		sprite.setMaterial(material);

		auto timeComp = new TimeComponent();
		timeComp->elapsed = r.getFloat(0.0f, 1.0f);

		world->createEntity()
			.addComponent(posComp)
			.addComponent(velComp)
			.addComponent(spriteComp)
			.addComponent(timeComp)
			.getEntityId();
	}

	target = getAPI().video->createRenderTarget();
	TextureDescriptor desc;
	desc.w = 1280;
	desc.h = 720;
	desc.format = TextureFormat::RGBA;
	target->setTarget(0, getAPI().video->createTexture(desc));
	target->unbind();
}

void TestStage::deInit()
{
}

void TestStage::onVariableUpdate(Time)
{
}

void TestStage::onFixedUpdate(Time time)
{
	world->step(TimeLine::FixedUpdate, time);
}

void TestStage::onRender(RenderContext& context) const
{
	context.bind([&] (Painter& painter)
	{
		painter.clear(Colour(0.2f, 0.2f, 0.3f));
		world->render(painter);
	});
}

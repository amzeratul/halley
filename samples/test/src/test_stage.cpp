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

	auto posComp = new PositionComponent();
	posComp->position = Vector2f(100, 100);

	auto velComp = new VelocityComponent();
	velComp->velocity = Vector2f(200, 200);

	auto spriteComp = new SpriteComponent();
	auto& sprite = spriteComp->sprite;
	auto spriteSheet = getResource<SpriteSheet>("sprites/ella.json");
	auto material = getResource<Material>("shaders/sprite.yaml");
	(*material)["tex0"] = spriteSheet->getTexture();
	sprite.setMaterial(material);

	auto timeComp = new TimeComponent();
	timeComp->elapsed = 0;

	id0 = world->createEntity()
		.addComponent(posComp)
		.addComponent(velComp)
		.addComponent(spriteComp)
		.addComponent(timeComp)
		.getEntityId();

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
	i++;

	if (i == 100) {
	}

	curTime += float(time);
}

void TestStage::onRender(RenderContext& context) const
{
	context.bind([&] (Painter& painter)
	{
		painter.clear(Colour(0.2f, 0.2f, 0.3f));
		world->render(painter);
	});
}

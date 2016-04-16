#include "test_stage.h"
#include <gen/cpp/systems/test_system.h>
#include <gen/cpp/systems/render_system.h>

using namespace Halley;

void TestStage::init()
{
	world.addSystem(std::make_unique<TestSystem>(), TimeLine::FixedUpdate);
	world.addSystem(std::make_unique<RenderSystem>(), TimeLine::Render);

	id0 = world.createEntity()
		.addComponent(new TestComponent())
		.addComponent(new FooComponent())
		.addComponent(new BarComponent())
		.getEntityId();

	auto spriteSheet = getResource<SpriteSheet>("sprites/ella.json");
	auto material = getResource<Material>("shaders/sprite.yaml");
	(*material)["tex0"] = spriteSheet->getTexture();
	sprite.setMaterial(material);
	sprite.setPos(Vector2f(100, 100));
	sprite.setSprite(*spriteSheet, "die/05.png");

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
	std::cout << "Final bar: " << world.getEntity(id0).getComponent<BarComponent>()->bar << std::endl;
}

void TestStage::onVariableUpdate(Time)
{
}

void TestStage::onFixedUpdate(Time time)
{
	if (i == 20) {
		world.createEntity()
			.addComponent(new TestComponent())
			.addComponent(new FooComponent());
	}
	if (i == 40) {
		id2 = world.createEntity()
			.addComponent(new TestComponent())
			.addComponent(new BarComponent())
			.getEntityId();
	}
	if (i == 60) {
		world.getEntity(id2).removeComponent<TestComponent>();
	}
	if (i == 80) {
		world.destroyEntity(id2);
	}

	world.step(TimeLine::FixedUpdate, time);
	i++;

	if (i == 100) {
		//getAPI().core->quit();
	}

	curTime += float(time);

	sprite.setPos(Vector2f(640, 360) + Vector2f(500, 0) * ::sin(curTime * 0.2f));
	sprite.setColour(Colour4f(1, 1, 1, ::sin(curTime * 2) * 0.5f + 0.5f));
}

void TestStage::onRender(RenderContext& context) const
{
	sprite.getMaterial()["u_time"] = curTime;

	context.bind([&] (Painter& painter)
	{
		painter.clear(Colour(0.2f, 0.2f, 0.3f));
		world.render(painter);

		sprite.draw(painter);
	});
}

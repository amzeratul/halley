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
	}

	curTime += float(time);


	//const char* ellaFrames[] = { "die/01.png", "die/02.png", "die/03.png", "die/04.png", "die/05.png", "idle_back/01.png", "idle_back/02.png", "idle_back/03.png", "idle_front/01.png", "idle_front/02.png", "idle_front/03.png", "idle_side/01.png", "idle_side/02.png", "idle_side/03.png", "run_back/01.png", "run_back/02.png", "run_back/03.png", "run_back/04.png", "run_front/01.png", "run_front/02.png", "run_front/03.png", "run_front/04.png", "run_side/01.png", "run_side/02.png", "run_side/03.png", "run_side/04.png" };
	//const char* ellaFrames[] = { "run_side/01.png", "run_side/02.png", "run_side/03.png", "run_side/04.png" };
	const char* ellaFrames[] = { "die/01.png", "die/02.png", "die/03.png", "die/04.png", "die/05.png" };
	size_t nFrames = sizeof(ellaFrames) / sizeof(const char*);
	size_t curFrame = size_t(curTime * 5) % nFrames;
	sprite.setFlip(int(curTime) % 2 == 0);

	sprite.setSprite(*getResource<SpriteSheet>("sprites/ella.json"), ellaFrames[curFrame]);
}

void TestStage::onRender(RenderContext& context) const
{
	context.bind([&] (Painter& painter)
	{
		painter.clear(Colour(0.2f, 0.2f, 0.3f));
		world.render(painter);

		sprite.draw(painter);
	});
}

#include "test_stage.h"
#include "../gen/cpp/registry.h"
#include "../gen/cpp/components/position_component.h"
#include "../gen/cpp/components/sprite_animation_component.h"
#include "../gen/cpp/components/sprite_component.h"
#include "../gen/cpp/components/time_component.h"
#include "../gen/cpp/components/velocity_component.h"

using namespace Halley;

void TestStage::init()
{
	world = std::make_unique<World>(&getAPI());
	world->addSystem(createSystem("TimeSystem"), TimeLine::FixedUpdate);
	world->addSystem(createSystem("MovementSystem"), TimeLine::FixedUpdate);
	world->addSystem(createSystem("SpriteAnimationSystem"), TimeLine::FixedUpdate);
	world->addSystem(createSystem("RenderSystem"), TimeLine::Render);

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
#ifdef _DEBUG
	const int targetEntities = 20;
#else
	const int targetEntities = 10000;
#endif
	const int nToSpawn = std::min(targetEntities - int(world->numEntities()), std::max(1, targetEntities / 60));
	for (int i = 0; i < nToSpawn; i++) {
		spawnTestSprite();
	}

	world->step(TimeLine::FixedUpdate, time);

	if (getAPI().input->getKeyboard().isButtonDown(Keys::Esc)) {
		getAPI().core->quit();
	}
}

void TestStage::onRender(RenderContext& context) const
{
	Sprite sprite;
	auto texture = getResource<Texture>("sprites/test_distance_field.png");
	auto material = getResource<Material>("shaders/distance_field_sprite.yaml");
	(*material)["tex0"] = texture;
	sprite.setMaterial(material);
	sprite.setPivot(Vector2f(0.5f, 0.5f));
	sprite.setPos(Vector2f(640, 360));
	sprite.setSize(Vector2f(1024, 1024));
	sprite.setTexRect(Rect4f(0, 0, 1, 1));
	sprite.setColour(Colour4f(0.6f, 0.1f, 0.1f, 1));

	context.bind([&] (Painter& painter)
	{
		painter.clear(Colour(0.2f, 0.2f, 0.3f));
		world->render(painter);

		//sprite.draw(painter);
	});
}

void TestStage::spawnTestSprite()
{
	auto& r = Random::getGlobal();

	auto posComp = new PositionComponent();
	posComp->position = Vector2f(r.getFloat(0.0f, 1280.0f), r.getFloat(0.0f, 720.0f));

	auto velComp = new VelocityComponent();
	velComp->velocity = Vector2f(r.getFloat(200.0f, 300.0f), 0.0f).rotate(Angle1f::fromDegrees(r.getFloat(0.0f, 360.0f)));

	auto spriteComp = new SpriteComponent();

	auto timeComp = new TimeComponent();
	timeComp->elapsed = r.getFloat(0.0f, 1.0f);

	auto animComp = new SpriteAnimationComponent();
	animComp->player.setAnimation(getResource<Animation>("animations/ella.yaml"));
	animComp->player.setSequence("run");

	world->createEntity()
		.addComponent(posComp)
		.addComponent(velComp)
		.addComponent(spriteComp)
		.addComponent(timeComp)
		.addComponent(animComp);
}

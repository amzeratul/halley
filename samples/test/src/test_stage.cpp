#include "test_stage.h"
#include "registry.h"
#include "components/sprite_component.h"
#include "components/position_component.h"

using namespace Halley;

void TestStage::init()
{
	world = createWorld("sample_test_world.yaml", createSystem);

	//target = getAPI().video->createRenderTarget();
	//target->setTarget(0, getAPI().video->createTexture(TextureDescriptor(Vector2i(1280, 720))));

	auto sprite = Sprite()
		.setImage(getAPI().core->getResources(), "halley_logo_dist.png", "distance_field_sprite.yaml")
		.setPivot(Vector2f(0.0f, 1.0f))
		.setColour(Colour4f(0.9882f, 0.15686f, 0.27843f, 1))
		.setScale(Vector2f(2, 2));
	world->createEntity()
		.addComponent(new SpriteComponent(sprite, 1))
		.addComponent(new PositionComponent(Vector2f(32, 752)));
}

void TestStage::deInit()
{
}

void TestStage::onFixedUpdate(Time time)
{
	world->step(TimeLine::FixedUpdate, time);

	if (getAPI().input->getKeyboard().isButtonDown(Keys::Esc)) {
		getAPI().core->quit();
	}
}

void TestStage::onRender(RenderContext& context) const
{
	context.bind([&] (Painter& painter)
	{
		painter.clear(Colour(0.2f, 0.2f, 0.3f));
		world->render(painter);
	});
}

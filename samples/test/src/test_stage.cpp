#include "test_stage.h"
#include "../gen/cpp/registry.h"

using namespace Halley;

void TestStage::init()
{
	world = createWorld("sample_test_world.yaml", createSystem);

	target = getAPI().video->createRenderTarget();
	target->setTarget(0, getAPI().video->createTexture(TextureDescriptor(Vector2i(1280, 720))));

	halleyLogo = Sprite()
		.setImage(getAPI().core->getResources(), "halley_logo_dist.png", "distance_field_sprite.yaml")
		.setPivot(Vector2f(0.0f, 1.0f))
		.setPos(Vector2f(0, 720) + Vector2f(32, 32))
		.setColour(Colour4f(0.9882f, 0.15686f, 0.27843f, 1))
		.setScale(Vector2f(2, 2));
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
		halleyLogo.draw(painter);
	});
}

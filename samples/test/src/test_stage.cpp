#include "test_stage.h"
#include "registry.h"
#include "components/sprite_component.h"
#include "components/position_component.h"

using namespace Halley;

void TestStage::init()
{
	world = createWorld("sample_test_world.yaml", createSystem);
	statsView = std::make_unique<WorldStatsView>(*getAPI().core, *world);

	//target = getAPI().video->createRenderTarget();
	//target->setTarget(0, getAPI().video->createTexture(TextureDescriptor(Vector2i(1280, 720))));

	auto col = Colour4f(0.9882f, 0.15686f, 0.27843f, 1);
	auto sprite = Sprite()
		.setImage(getResources(), "halley_logo_dist.png", "distance_field_sprite.yaml")
		.setPivot(Vector2f(0.0f, 1.0f))
		.setColour(col)
		.setScale(Vector2f(2, 2));
	sprite.getMaterial()["u_smoothness"] = 0.1f;
	sprite.getMaterial()["u_outline"] = 0.0f;
	sprite.getMaterial()["u_outlineColour"] = col;

	world->createEntity()
		.addComponent(new SpriteComponent(sprite, 1))
		.addComponent(new PositionComponent(Vector2f(32, 752)));
}

void TestStage::onFixedUpdate(Time time)
{
	world->step(TimeLine::FixedUpdate, time);

	auto& key = getInputAPI().getKeyboard();
	if (key.isButtonDown(Keys::Esc)) {
		getCoreAPI().quit();
	}
}

void TestStage::onRender(RenderContext& context) const
{
	context.bind([&] (Painter& painter)
	{
		painter.clear(Colour(0.2f, 0.2f, 0.3f));
		world->render(painter);
	});

	statsView->draw(context);
}

/*
void TestStage::onVariableUpdate(Halley::Time time)
{
	std::cout << ":D ";
}
*/
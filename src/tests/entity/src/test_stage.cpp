#include "test_stage.h"
#include "registry.h"

using namespace Halley;

void TestStage::init()
{
	world = createWorld("sample_test_world", createSystem);
	statsView = std::make_unique<WorldStatsView>(*getAPI().core);
	statsView->setWorld(world.get());
}

void TestStage::onFixedUpdate(Time time)
{
	world->step(TimeLine::FixedUpdate, time);
}

void TestStage::onVariableUpdate(Time time)
{
	world->step(TimeLine::VariableUpdate, time);

	auto key = getInputAPI().getKeyboard();
	if (key->isButtonDown(Keys::Esc)) {
		getCoreAPI().quit();
	}
}

void TestStage::onRender(RenderContext& context) const
{
	world->render(context);
	statsView->draw(context);
}

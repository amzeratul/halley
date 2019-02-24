#include "test_stage.h"
#include "registry.h"

using namespace Halley;

void TestStage::init()
{
	world = createWorld("sample_test_world", createSystem);
	statsView = std::make_unique<WorldStatsView>(*getAPI().core);
	auto stolen = world.release();
	world.reset(stolen);
	statsView->setWorld(stolen);
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

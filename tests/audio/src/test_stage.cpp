#include "test_stage.h"

using namespace Halley;

void TestStage::init()
{
	getAudioAPI().playMusic(getResource<AudioClip>("Loveshadow_-_Marcos_Theme.ogg"));
}

void TestStage::onVariableUpdate(Time time)
{
	auto key = getInputAPI().getKeyboard();
	if (key->isButtonDown(Keys::Esc)) {
		getCoreAPI().quit();
	}
}

void TestStage::onRender(RenderContext& context) const
{
}

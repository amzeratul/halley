#include "halley/stage/render_test_stage.h"

#include "halley/graphics/render_context.h"
#include "halley/graphics/render_target/render_target.h"

using namespace Halley;


void IRenderTest::init(Resources& resources, const HalleyAPI& api, SpritePainter& spritePainter)
{
	this->resources = &resources;
	this->api = &api;
	this->spritePainter = &spritePainter;
	onInit();
}

void IRenderTest::render(RenderContext& rc) const
{
	rc.bind([&](Painter& painter)
	{
		spritePainter->draw(1, painter);
	});
}

void SSBOTest::onInit()
{
	sprite = Sprite().setImage(*resources, "ui/whitebox.png", "Halley/SpriteSSBOTest")
		.setColour(Colour4f(1, 1, 1));
}

void SSBOTest::update(Time dt, Time totalTime)
{
	auto screenSize = Vector2f(api->video->getWindow().getDefinition().getSize());

	sprite
		.setPosition(screenSize / 2)
		.setPivot(Vector2f(0.5f, 0.5f))
		.setScale(10);

	struct Data {
		Vector4f col;
	};
	float phase = static_cast<float>(std::fmod(totalTime / 5.0, 1.0f));
	std::array<Data, 4> sd;
	for (int i = 0; i < 4; ++i) {
		sd[i].col = Colour4f::fromHSV(std::fmod(phase + (i * 0.25f), 1.0f), 1, 1, 1).toVector4();
	}
	sprite.getMutableMaterial().setStructuredBuffer("SpriteData", as_bytes(gsl::span<Data>(sd)));

	spritePainter->add(sprite, 1, 0, {});
}



void RenderTestStage::init()
{
	setTestType(TestType::SSBOTest);
}

void RenderTestStage::onVariableUpdate(Time dt)
{
	spritePainter.startFrame(hasMultithreadedRendering());
	if (curTest) {
		curTest->update(dt, elapsedTime);
	}
	elapsedTime += dt;
}

void RenderTestStage::onRender(RenderContext& rc) const
{
	spritePainter.startRender(true, false, {});
	if (curTest) {
		Camera camera;
		const auto viewPort = rc.getDefaultRenderTarget().getViewPort();
		camera.setPosition(Rect4f(viewPort).getCenter());
		camera.setViewPort(viewPort);
		auto rc2 = rc.with(camera);
		curTest->render(rc2);
	}
	spritePainter.endRender();
}

bool RenderTestStage::hasMultithreadedRendering() const
{
	return false;
}

void RenderTestStage::setTestType(TestType testType)
{
	curTest = {};
	curTest = makeTest(testType);
	if (curTest) {
		curTest->init(getResources(), getAPI(), spritePainter);
	}
}

std::unique_ptr<IRenderTest> RenderTestStage::makeTest(TestType testType) const
{
	switch (testType) {
	case TestType::SSBOTest:
		return std::make_unique<SSBOTest>();
	}
	return {};
}

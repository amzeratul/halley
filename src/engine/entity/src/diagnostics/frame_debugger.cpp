#include "diagnostics/frame_debugger.h"

#include "halley/core/resources/resources.h"
using namespace Halley;

FrameDebugger::FrameDebugger(Resources& resources, const HalleyAPI& api)
	: StatsView(resources, api)
{
	headerText = TextRenderer(resources.get<Font>("Ubuntu Bold"), "", 16, Colour(1, 1, 1), 1.0f, Colour(0.1f, 0.1f, 0.1f));
}

FrameDebugger::~FrameDebugger()
{
}

void FrameDebugger::update()
{
	StatsView::update();
}

void FrameDebugger::paint(Painter& painter)
{
	if (!isActive()) {
		return;
	}

	headerText
		.setPosition(Vector2f(10, 10))
		.setOffset(Vector2f())
		.setOutline(1.0f)
		.setText("Halley Frame Debugger (WIP)")
		.draw(painter);
}

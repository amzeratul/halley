#include "diagnostics/frame_debugger.h"

#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/painter.h"
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
	if (!isActive()) {
		renderSnapshot = {};
		waiting = false;
		return;
	}

	if (!renderSnapshot && !waiting) {
		waiting = true;
		api.core->requestRenderSnapshot().then(Executors::getMainUpdateThread(), [&] (std::unique_ptr<RenderSnapshot> snapshot)
		{
			if (waiting) {
				renderSnapshot = std::move(snapshot);
				waiting = false;
			}
		});
	}
}

void FrameDebugger::draw(RenderContext& context)
{
	if (isRendering()) {
		context.bind([&](Painter& painter)
		{
			renderSnapshot->playback(painter, {});
		});
	}

	StatsView::draw(context);
}

void FrameDebugger::paint(Painter& painter)
{
	if (!isActive()) {
		return;
	}

	headerText
		.setPosition(Vector2f(10, 10))
		.setOffset(Vector2f())
		.setOutline(1.0f);

	if (renderSnapshot) {
		headerText
			.setText("Halley Frame Debugger - Got " + toString(renderSnapshot->getNumCommands()) + " commands recorded.")
			.draw(painter);
	} else {
		headerText
			.setText("Halley Frame Debugger - Waiting for snapshot...")
			.draw(painter);
	}
}

bool FrameDebugger::isRendering() const
{
	return renderSnapshot && isActive();
}

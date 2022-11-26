#include "diagnostics/frame_debugger.h"

#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/resources/resources.h"
#include "halley/support/logger.h"
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
				Logger::logDev("Captured frame in frame debugger");
				renderSnapshot = std::move(snapshot);
				framesToDraw = renderSnapshot->getNumCommands();
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
			painter.stopRecording();
			painter.clear(Colour4f());

			n++;
			framesToDraw = n % (renderSnapshot->getNumCommands() + 1);
			renderSnapshot->playback(painter, framesToDraw);
		});
	}

	StatsView::draw(context);
}

void FrameDebugger::paint(Painter& painter)
{
	if (!isActive()) {
		return;
	}
	painter.stopRecording();

	headerText
		.setPosition(Vector2f(10, 50))
		.setOffset(Vector2f())
		.setOutline(1.0f);

	if (renderSnapshot) {
		headerText
			.setText("Halley Frame Debugger\nDraw call " + toString(framesToDraw) + "/" + toString(renderSnapshot->getNumCommands()))
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

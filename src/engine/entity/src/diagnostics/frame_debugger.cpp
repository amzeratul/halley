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

void FrameDebugger::update(Time t)
{
	if (!isActive()) {
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
				framesToDraw = static_cast<int>(renderSnapshot->getNumCommands());
				waiting = false;
			}
		});
	}

	if (renderSnapshot) {
		const int dy = input->getAxisRepeat(1);
		framesToDraw = clamp(framesToDraw + dy, 0, static_cast<int>(renderSnapshot->getNumCommands()));
	}
}

void FrameDebugger::draw(RenderContext& context)
{
	if (isRendering()) {
		context.bind([&](Painter& painter)
		{
			painter.stopRecording();
			painter.clear(Colour4f());
			
			renderSnapshot->playback(painter, framesToDraw);
		});
	}

	StatsView::draw(context);
}

void FrameDebugger::paint(Painter& painter)
{
	if (!isActive()) {
		renderSnapshot = {};
		return;
	}
	painter.stopRecording();

	headerText
		.setPosition(Vector2f(10, 50))
		.setOffset(Vector2f())
		.setOutline(1.0f);

	ColourStringBuilder str;
	str.append("Halley Frame Debugger\n");

	if (renderSnapshot) {
		str.append("Draw call " + toString(framesToDraw) + " / " + toString(renderSnapshot->getNumCommands()));
	} else {
		str.append("Waiting for snapshot...");
	}

	auto [strText, strCol] = str.moveResults();
	headerText
		.setText(std::move(strText))
		.setColourOverride(std::move(strCol))
		.draw(painter);
}

bool FrameDebugger::isRendering() const
{
	return renderSnapshot && isActive();
}

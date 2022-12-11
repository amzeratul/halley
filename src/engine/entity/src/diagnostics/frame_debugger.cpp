#include "diagnostics/frame_debugger.h"

#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/graphics/render_context.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/render_snapshot.h"
#include "halley/core/resources/resources.h"
#include "halley/support/logger.h"
using namespace Halley;

FrameDebugger::FrameDebugger(Resources& resources, const HalleyAPI& api)
	: StatsView(resources, api)
	, yAxisRepeater(0.2, 0.1, 1.0, 0)
{
	headerText = TextRenderer(resources.get<Font>("Ubuntu Bold"), "", 16, Colour(1, 1, 1), 1.0f, Colour(0.1f, 0.1f, 0.1f));
	whitebox = Sprite().setImage(resources, "whitebox.png");
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
		api.core->requestRenderSnapshot().then(Executors::getMainUpdateThread(), [&](std::unique_ptr<RenderSnapshot> snapshot)
		{
			if (waiting) {
				assert(!snapshot->hasPendingTimestamps());
				renderSnapshot = std::move(snapshot);
				framesToDraw = static_cast<int>(renderSnapshot->getNumCommands());
				waiting = false;
			}
		});
	}

	if (renderSnapshot) {
		const int dy = yAxisRepeater.update(input->getAxis(1), t);
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

			if (!debugMaterial && resources.getOptions().retainShaderData) {
				//debugMaterial = resources.get<MaterialDefinition>("Halley/FrameDebugView");
			}
			
			lastPlaybackResult = renderSnapshot->playback(painter, framesToDraw, debugMaterial);
		});
	}

	StatsView::draw(context);
}

namespace {
	static String getCommandType(RenderSnapshot::CommandType type)
	{
		switch (type) {
		case RenderSnapshot::CommandType::Draw: return "Draw";
		case RenderSnapshot::CommandType::Clear: return "Clear";
		case RenderSnapshot::CommandType::Bind: return "Bind";
		case RenderSnapshot::CommandType::Unbind: return "Unbind";
		case RenderSnapshot::CommandType::SetClip: return "Clip";
		case RenderSnapshot::CommandType::Undefined: return "?";
		}
		return "";
	}

	static String getReason(RenderSnapshot::Reason reason)
	{
		switch (reason) {
		case RenderSnapshot::Reason::First: return "First";
		case RenderSnapshot::Reason::Clear: return "Clear";
		case RenderSnapshot::Reason::AfterClear: return "After Clear";
		case RenderSnapshot::Reason::ChangeBind: return "Binding Changed";
		case RenderSnapshot::Reason::ChangeClip: return "Clip Changed";
		case RenderSnapshot::Reason::MaterialDefinition: return "Material Definition Changed";
		case RenderSnapshot::Reason::MaterialParameters: return "Material Parameters Changed";
		case RenderSnapshot::Reason::Textures: return "Textures Changed";
		case RenderSnapshot::Reason::Unknown: return "?";
		}
		return "";
	}
}

void FrameDebugger::paint(Painter& painter)
{
	if (!isActive()) {
		renderSnapshot = {};
		return;
	}
	painter.stopRecording();

	ColourStringBuilder str;

	if (renderSnapshot) {
		const auto green = Colour4f::fromString("#69E479");
		const auto red = Colour4f::fromString("#E66F68");

		auto [frameStart, frameEnd] = renderSnapshot->getFrameTimeRange();
		if (frameEnd != frameStart) {
			const auto time = (frameEnd - frameStart) / 1000;
			str.append("Total Frame Time: ");
			str.append(toString(time) + " us\n", green);
		}

		str.append("Command " + toString(framesToDraw) + " / " + toString(renderSnapshot->getNumCommands()) + "\n");
		if (framesToDraw > 0) {
			const auto frameIdx = framesToDraw - 1;
			const auto info = renderSnapshot->getCommandInfo(frameIdx);

			str.append(getCommandType(info.type), green);
			str.append(" [");
			str.append(getReason(info.reason), green);
			str.append("]");
			str.append("\nRender target: ");
			str.append(lastPlaybackResult.finalRenderTargetName, info.hasBindChange ? red : green);

			if (info.type == RenderSnapshot::CommandType::Draw) {
				str.append("\nPolygons: ");
				str.append(toString(info.numTriangles), green);
				str.append(info.numTriangles == 1 ? " triangle" : " triangles");
				str.append("\nMaterial: ");
				str.append(info.materialDefinition, info.hasMaterialDefChange ? red : green);
				str.append(" [");
				str.append("0x" + toString(info.materialHash, 16, 16).asciiUpper(), info.hasMaterialParamsChange ? red : green);
				str.append("]\nTextures: ");
				str.append(String::concatList(info.textures, ", "), info.hasTextureChange ? red : green);
			} else if (info.type == RenderSnapshot::CommandType::Clear) {
				if (info.clearData->colour) {
					str.append("\nClear Colour: ");
					str.append(info.clearData->colour->toString(), green);
				}
				if (info.clearData->depth) {
					str.append("\nClear Depth: ");
					str.append(toString(*info.clearData->depth), green);
				}
				if (info.clearData->stencil) {
					str.append("\nClear Stencil: ");
					str.append("0x" + toString(int(*info.clearData->stencil), 16, 2).asciiUpper(), green);
				}
			}

			if (static_cast<int>(renderSnapshot->getNumTimestamps()) > frameIdx) {
				const auto [startTime, endTime, setupTime] = renderSnapshot->getCommandTimeStamp(frameIdx);
				const auto [prevStartTime, prevEndTime, prevSetupTime] = frameIdx > 0 ? renderSnapshot->getCommandTimeStamp(frameIdx - 1) : RenderSnapshot::CommandTimeStamp();

				const uint64_t commandLen = (endTime - startTime) / 1000;
				const uint64_t exclusiveCommandLen = (endTime - std::max(startTime, prevEndTime)) / 1000;
				const uint64_t setupLen = (setupTime - startTime) / 1000;
				const uint64_t gapLen = (startTime - std::min(startTime, prevEndTime)) / 1000;
				
				str.append("\nTime: ");
				str.append(toString(startTime / 1000) + " us", green);
				str.append(" to ");
				str.append(toString(endTime / 1000) + " us", green);
				str.append("\nDuration: ");
				str.append(toString(exclusiveCommandLen) + " us", green);
				str.append(" (");
				str.append(toString(setupLen) + " us", green);
				str.append(" setup, ");
				str.append(toString(commandLen) + " us", green);
				str.append(" total, ");
				str.append(toString(gapLen) + " us", gapLen > 0 ? red : green);
				str.append(" gap)");
			}
		}
	} else {
		str.append("Waiting for snapshot...");
	}

	auto [strText, strCol] = str.moveResults();
	const auto textPos = Vector2f(10, 10);
	headerText
		.setPosition(textPos)
		.setOffset(Vector2f())
		.setOutline(1.0f)
		.setText(std::move(strText))
		.setColourOverride(std::move(strCol));

	const auto textBounds = Rect4f(textPos, textPos + headerText.getExtents()).grow(5);

	whitebox.clone()
		.setPos(textBounds.getTopLeft())
		.scaleTo(textBounds.getSize())
		.setColour(Colour4f(0, 0, 0, 0.6f))
		.draw(painter);

	headerText.draw(painter);
}

bool FrameDebugger::isRendering() const
{
	return renderSnapshot && isActive();
}

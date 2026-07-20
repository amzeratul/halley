#include "halley/entity/services/debug_draw_service.h"

#include "halley/game/frame_data.h"
#include "halley/graphics/painter.h"
#include "halley/maths/polygon.h"
#include "halley/scripting/script_graph.h"
#include "halley/scripting/script_state.h"
#include "halley/scripting/script_node_type.h"

using namespace Halley;

const Vector<DebugLine>& DebugDrawService::getDebugLines()
{
	return BaseFrameData::getCurrent().debugLines;
}

void DebugDrawService::addDebugLine(Vector<Vector2f> line, Colour4f colour, float thickness, bool loop, Painter::LineParameters params)
{
	BaseFrameData::getCurrent().debugLines.emplace_back(std::move(line), colour, thickness, loop, params);
}

void DebugDrawService::addDebugArrow(Vector2f from, Vector2f to, Colour4f colour, float headSize, float thickness, float sideShift)
{
	auto& lines = BaseFrameData::getCurrent().debugLines;
	const auto fwd = (to - from).normalized();
	const auto right = fwd.orthoRight();

	const auto p0 = to + right * sideShift;
	const auto p3 = from + right * sideShift;
	const auto p1 = p0 - (headSize * fwd) + (headSize * 0.25f * right);
	const auto p2 = p0 - (headSize * fwd) - (headSize * 0.25f * right);

	lines.emplace_back(Vector<Vector2f>{{ p3, p0 }}, colour, thickness, false);
	lines.emplace_back(Vector<Vector2f>{{ p1, to }}, colour, thickness, false);
	lines.emplace_back(Vector<Vector2f>{{ p2, to }}, colour, thickness, false);
}

const Vector<DebugPoint>& DebugDrawService::getDebugPoints()
{
	return BaseFrameData::getCurrent().debugPoints;
}

void DebugDrawService::addDebugPoint(Vector2f point, Colour4f colour, float radius)
{
	BaseFrameData::getCurrent().debugPoints.emplace_back(point, colour, radius);
}

const Vector<DebugPolygon>& DebugDrawService::getDebugPolygons()
{
	return BaseFrameData::getCurrent().debugPolygons;
}

void DebugDrawService::addDebugPolygon(Polygon polygon, Colour4f colour)
{
	BaseFrameData::getCurrent().debugPolygons.emplace_back(std::move(polygon), colour);
}

const Vector<DebugEllipse>& DebugDrawService::getDebugEllipses()
{
	return BaseFrameData::getCurrent().debugEllipses;
}

const Vector<DebugWorldText>& DebugDrawService::getDebugWorldTexts()
{
	return BaseFrameData::getCurrent().debugWorldTexts;
}

void DebugDrawService::addDebugEllipse(Vector2f point, Vector2f radius, Colour4f colour, float thickness, Painter::LineParameters params)
{
	BaseFrameData::getCurrent().debugEllipses.emplace_back(point, radius, thickness, colour, params);
}

const Vector<std::pair<String, DebugText>>& DebugDrawService::getDebugTexts()
{
	return BaseFrameData::getCurrent().debugTexts;
}

void DebugDrawService::addDebugText(std::string_view key, String value, Time time)
{
	if (BaseFrameData::hasCurrent()) {
		auto& debugTexts = BaseFrameData::getCurrent().debugTexts;
		const auto iter = std_ex::find_if(debugTexts, [&] (const auto& e) { return e.first == key; });
		if (iter != debugTexts.end()) {
			iter->second.text = std::move(value);
			iter->second.time = time;
		} else {
			debugTexts.emplace_back(key, DebugText(std::move(value), time));
		}
	}
}

void DebugDrawService::addDebugText(String value, Vector2f position)
{
	BaseFrameData::getCurrent().debugWorldTexts.emplace_back(value, position);
}

void DebugDrawService::addScriptRenderer(Vector2f pos, std::shared_ptr<ScriptState> state)
{
	// Make a copy of the state
	BaseFrameData::getCurrent().scriptStates.emplace_back(pos, std::make_shared<ScriptState>(*state));
}

void DebugDrawService::drawScripts(Painter& painter)
{
	for (const auto& s: BaseFrameData::getCurrent().scriptStates) {
		const float posScale = s.second->getScriptGraphPtr()->getAssetId().isEmpty() ? 1.0f : 0.5f;
		const Vector2f pos = s.first - s.second->getDisplayOffset() * posScale;
		scriptGraphRenderer->setGraph(s.second->getScriptGraphPtr());
		scriptGraphRenderer->setState(s.second.get());
		scriptGraphRenderer->draw(painter, pos, painter.getCurrentCamera().getZoom(), posScale);
	}
}

void DebugDrawService::initScriptGraphRenderer(Resources& resources, const ScriptNodeTypeCollection& scriptNodeTypeCollection, float nativeZoom)
{
	scriptGraphRenderer = std::make_unique<ScriptRenderer>(resources, scriptNodeTypeCollection, nativeZoom);
}

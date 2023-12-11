#include "halley/entity/services/dev_service.h"

#include "halley/game/frame_data.h"
#include "halley/graphics/painter.h"
#include "halley/maths/polygon.h"
#include "halley/ui/widgets/ui_debug_console.h"

using namespace Halley;

DevService::DevService(bool editorMode, bool devMode, std::shared_ptr<Options> options)
	: options(std::move(options))
	, editorMode(editorMode)
	, devMode(devMode)
{
	commands = std::make_shared<UIDebugConsoleCommands>();
}

bool DevService::isDevMode() const
{
	return devMode;
}

bool DevService::isEditorMode() const
{
	return editorMode;
}

void DevService::setCommands(std::shared_ptr<UIDebugConsoleCommands> commands)
{
	this->commands = std::move(commands);
}

UIDebugConsoleCommands& DevService::getConsoleCommands()
{
	return *commands;
}

void DevService::setTime(Time t)
{
	time = t;
}

Time DevService::getTime() const
{
	return time;
}

void DevService::setEditorTool(String tool)
{
	editorTool = std::move(tool);
}

const String& DevService::getEditorTool() const
{
	return editorTool;
}

void DevService::setCurrentEntitySubWorld(std::optional<int> subWorld)
{
	curEntitySubWorld = subWorld;
}

void DevService::setSubWorldDisplayOverride(std::optional<int> subWorld)
{
	subWorldDisplayOverride = subWorld;
}

std::optional<int> DevService::getSubWorldDisplayOverride() const
{
	if (subWorldDisplayOverride) {
		return subWorldDisplayOverride;
	} else {
		return curEntitySubWorld;
	}
}

void DevService::setSelectedEntities(Vector<EntityId> ids)
{
	selected = std::move(ids);
}

const Vector<EntityId>& DevService::getSelectedEntities() const
{
	return selected;
}

void DevService::setSceneBoundaries(Polygon boundaries, Polygon safeBoundaries)
{
	sceneBoundaries = std::move(boundaries);
	safeSceneBoundaries = std::move(safeBoundaries);
}

void DevService::setAltSceneBoundaries(Polygon boundaries)
{
	altSceneBoundaries = std::move(boundaries);
}

const Polygon& DevService::getSceneBoundaries() const
{
	return sceneBoundaries;
}

const Polygon& DevService::getSafeSceneBoundaries() const
{
	return safeSceneBoundaries;
}

const Polygon& DevService::getAltSceneBoundaries() const
{
	return altSceneBoundaries;
}

bool DevService::hasSceneBoundaries() const
{
	return !sceneBoundaries.getVertices().empty();
}

bool DevService::hasAltSceneBoundaries() const
{
	return !altSceneBoundaries.getVertices().empty();
}

bool DevService::getAllowCameraTargets() const
{
	return allowCameraTargets;
}

void DevService::setAllowCameraTargets(bool allowed)
{
	allowCameraTargets = allowed;
}

void DevService::setMousePos(std::optional<Vector2f> pos)
{
	mousePos = pos;
}

std::optional<Vector2f> DevService::getMousePos() const
{
	return mousePos;
}

const Vector<DebugLine>& DevService::getDebugLines()
{
	return BaseFrameData::getCurrent().debugLines;
}

void DevService::addDebugLine(Vector<Vector2f> line, Colour4f colour, float thickness, bool loop)
{
	BaseFrameData::getCurrent().debugLines.emplace_back(std::move(line), colour, thickness, loop);
}

void DevService::addDebugArrow(Vector2f from, Vector2f to, Colour4f colour, float headSize, float thickness, float sideShift)
{
	auto& lines = BaseFrameData::getCurrent().debugLines;
	const auto fwd = (to - from).normalized();
	const auto right = fwd.orthoRight();

	const auto p0 = to + right * sideShift;
	const auto p3 = from + right * sideShift;
	const auto p1 = p0 - (headSize * fwd) + (headSize * 0.5f * right);
	const auto p2 = p0 - (headSize * fwd) - (headSize * 0.5f * right);

	lines.emplace_back(Vector<Vector2f>{{ p3, p0 }}, colour, thickness, false);
	lines.emplace_back(Vector<Vector2f>{{ p1, to }}, colour, thickness, false);
	lines.emplace_back(Vector<Vector2f>{{ p2, to }}, colour, thickness, false);
}

const Vector<DebugPoint>& DevService::getDebugPoints()
{
	return BaseFrameData::getCurrent().debugPoints;
}

void DevService::addDebugPoint(Vector2f point, Colour4f colour, float radius)
{
	BaseFrameData::getCurrent().debugPoints.emplace_back(point, colour, radius);
}

const Vector<DebugPolygon>& DevService::getDebugPolygons()
{
	return BaseFrameData::getCurrent().debugPolygons;
}

void DevService::addDebugPolygon(Polygon polygon, Colour4f colour)
{
	BaseFrameData::getCurrent().debugPolygons.emplace_back(std::move(polygon), colour);
}

const Vector<DebugEllipse>& DevService::getDebugEllipses()
{
	return BaseFrameData::getCurrent().debugEllipses;
}

const Vector<DebugWorldText>& DevService::getDebugWorldTexts()
{
	return BaseFrameData::getCurrent().debugWorldTexts;
}

void DevService::addDebugEllipse(Vector2f point, Vector2f radius, Colour4f colour, float thickness)
{
	BaseFrameData::getCurrent().debugEllipses.emplace_back(point, radius, thickness, colour);
}

const TreeMap<String, DebugText>& DevService::getDebugTexts()
{
	return BaseFrameData::getCurrent().debugTexts;
}

void DevService::addDebugText(std::string_view key, String value)
{
	auto& dt = BaseFrameData::getCurrent().debugTexts[key];
	dt.text = std::move(value);
	dt.time = 0.0;
}

void DevService::addDebugText(String value, Vector2f position)
{
	BaseFrameData::getCurrent().debugWorldTexts.emplace_back(value, position);
}

void DevService::addScriptRenderer(Vector2f pos, std::shared_ptr<ScriptState> state)
{
	// Make a copy of the state
	BaseFrameData::getCurrent().scriptStates.emplace_back(pos, std::make_shared<ScriptState>(*state));
}

void DevService::drawScripts(Painter& painter)
{
	for (const auto& s: BaseFrameData::getCurrent().scriptStates) {
		const float posScale = s.second->getScriptGraphPtr()->getAssetId().isEmpty() ? 1.0f : 0.5f;
		const Vector2f pos = s.first - s.second->getDisplayOffset() * posScale;
		scriptGraphRenderer->setGraph(s.second->getScriptGraphPtr());
		scriptGraphRenderer->setState(s.second.get());
		scriptGraphRenderer->draw(painter, pos, painter.getCurrentCamera().getZoom(), posScale);
	}
}

void DevService::initScriptGraphRenderer(Resources& resources, World& world, const ScriptNodeTypeCollection& scriptNodeTypeCollection, float nativeZoom)
{
	scriptGraphRenderer = std::make_unique<ScriptRenderer>(resources, &world, scriptNodeTypeCollection, nativeZoom);
}

void DevService::setDevValue(std::string_view name, float value)
{
	if (devMode) {
		options->setDevValue(name, value);
		options->save();
	}
}

float DevService::getDevValue(std::string_view name, float defaultValue) const
{
	if (devMode) {
		return options->getDevValue(name, defaultValue);
	} else {
		return defaultValue;
	}
}

void DevService::setDevFlag(std::string_view name, bool value)
{
	if (devMode) {
		options->setDevFlag(name, value);
		options->save();
	}
}

bool DevService::getDevFlag(std::string_view name, bool defaultValue) const
{
	if (devMode && options) {
		return options->getDevFlag(name, defaultValue);
	} else {
		return defaultValue;
	}
}

const std::optional<String>& DevService::getRenderNodeOverride() const
{
	return renderNodeOverride;
}

void DevService::setRenderNodeOverride(std::optional<String> node)
{
	renderNodeOverride = std::move(node);
}

void DevService::setDarkenChunks(bool value)
{
	darkenChunks = value;
}

bool DevService::getDarkenChunks() const
{
	return darkenChunks;
}

void DevService::setIsScene(bool isScene)
{
	sceneMode = isScene;
}

bool DevService::isScene() const
{
	return sceneMode;
}

void DevService::setEntitiesUnderCursor(const Vector<EntityRef>& entities)
{
	entitiesUnderCursor = &entities;
}

const Vector<EntityRef>* DevService::getEntitiesUnderCursor() const
{
	return entitiesUnderCursor;
}

#include "halley/entity/services/dev_service.h"

#include "halley/game/frame_data.h"
#include "halley/graphics/painter.h"
#include "halley/maths/polygon.h"
#include "halley/scripting/script_graph.h"
#include "halley/scripting/script_state.h"
#include "halley/ui/widgets/ui_debug_console.h"
#include "halley/graph/base_graph_type.h"
#include "halley/scripting/script_node_type.h"

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

void DevService::setDevValue(std::string_view name, float value)
{
	if (devMode) {
		options->setDevValue(name, value);
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

#include "scene_editor/component_editor_context.h"
#include "halley/file_formats/config_file.h"

using namespace Halley;

ComponentEditorContext::ComponentEditorContext(IEntityEditor& parent, UIFactory& factory, Resources& gameResources)
	: parent(parent)
	, factory(factory)
	, gameResources(gameResources)
{}

Resources& ComponentEditorContext::getGameResources() const
{
	return gameResources;
}

std::shared_ptr<IUIElement> ComponentEditorContext::makeLabel(const String& label) const
{
	return parent.makeLabel(label);
}

void ComponentEditorContext::setTool(SceneEditorTool tool, const String& componentName, const String& fieldName, ConfigNode options) const
{
	parent.setTool(tool, componentName, fieldName, std::move(options));
}

UIFactory& ComponentEditorContext::getFactory() const
{
	return factory;
}

void ComponentEditorContext::onEntityUpdated() const
{
	parent.onEntityUpdated();
}

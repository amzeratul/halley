#include "halley/editor_extensions/component_editor_context.h"
#include "halley/file_formats/config_file.h"
#include "halley/editor_extensions/component_field_parameters.h"

using namespace Halley;

ComponentEditorContext::ComponentEditorContext(IProjectWindow& projectWindow, IEntityEditorFactory& entityEditorFactory, IEntityEditorCallbacks* entityEditor, UIFactory& factory, Resources* gameResources)
	: projectWindow(projectWindow)
	, entityEditorFactory(&entityEditorFactory)
	, entityEditor(entityEditor)
	, factory(&factory)
	, gameResources(gameResources)
{
}

Resources& ComponentEditorContext::getGameResources() const
{
	Expects(gameResources != nullptr);
	return *gameResources;
}

IProjectWindow& ComponentEditorContext::getProjectWindow() const
{
	return projectWindow;
}

UIFactory& ComponentEditorContext::getUIFactory() const
{
	Expects(factory != nullptr);
	return *factory;
}

std::shared_ptr<IUIElement> ComponentEditorContext::makeLabel(const String& label) const
{
	Expects(entityEditorFactory != nullptr);
	return entityEditorFactory->makeLabel(label);
}

std::shared_ptr<IUIElement> ComponentEditorContext::makeField(const String& fieldType, ComponentFieldParameters parameters, ComponentEditorLabelCreation createLabel) const
{
	Expects(entityEditorFactory != nullptr);
	return entityEditorFactory->makeField(fieldType, std::move(parameters), createLabel);
}

ConfigNode ComponentEditorContext::getDefaultNode(const String& fieldType) const
{
	Expects(entityEditorFactory != nullptr);
	return entityEditorFactory->getDefaultNode(fieldType);
}

void ComponentEditorContext::setTool(const String& tool, const String& componentName, const String& fieldName) const
{
	if (entityEditor) {
		entityEditor->setTool(tool, componentName, fieldName);
	}
}

void ComponentEditorContext::setDefaultName(const String& name, const String& prevName) const
{
	if (entityEditor) {
		entityEditor->setDefaultName(name, prevName);
	}
}

IEntityEditorCallbacks* ComponentEditorContext::getEntityEditorCallbacks() const
{
	return entityEditor;
}

void ComponentEditorContext::onEntityUpdated() const
{
	if (entityEditor) {
		entityEditor->onEntityUpdated();
	}
}

#include "ui_editor.h"
using namespace Halley;

UIEditor::UIEditor(UIFactory& factory, Resources& gameResources, Project& project)
	: AssetEditor(factory, gameResources, project, AssetType::UIDefinition)
{
}

void UIEditor::reload()
{
	
}

void UIEditor::onMakeUI()
{
	
}

std::shared_ptr<const Resource> UIEditor::loadResource(const String& assetId)
{
	return gameResources.get<UIDefinition>(assetId);
}

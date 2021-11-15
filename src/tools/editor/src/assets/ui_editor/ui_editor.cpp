#include "ui_editor.h"

#include "halley/tools/project/project.h"
using namespace Halley;

UIEditor::UIEditor(UIFactory& factory, Resources& gameResources, Project& project, const HalleyAPI& api)
	: AssetEditor(factory, gameResources, project, AssetType::UIDefinition)
{
	gameI18N = std::make_unique<I18N>(gameResources, I18NLanguage("en-GB"));
	gameFactory = project.getGameInstance()->createUIFactory(api, gameResources, *gameI18N);
	factory.loadUI(*this, "halley/ui_editor");
}

void UIEditor::onMakeUI()
{
	display = getWidget("display");
	doLoadUI();
}

void UIEditor::reload()
{
	doLoadUI();
}

std::shared_ptr<const Resource> UIEditor::loadResource(const String& id)
{
	uiDefinition = gameResources.get<UIDefinition>(id);
	return uiDefinition;
}

void UIEditor::doLoadUI()
{
	if (uiDefinition && display) {
		display->clear();
		gameFactory->loadUI(*display, *uiDefinition);
	}
}

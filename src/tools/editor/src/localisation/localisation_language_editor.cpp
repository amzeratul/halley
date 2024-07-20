#include "localisation_language_editor.h"

#include "localisation_data.h"
#include "localisation_editor_root.h"

using namespace Halley;

LocalisationLanguageEditor::LocalisationLanguageEditor(LocalisationEditorRoot& root, Project& project, UIFactory& factory, LocalisationData& srcLanguage, LocalisationData* dstLanguage, bool canEdit)
	: UIWidget("localisation_language_editor", {}, UISizer())
	, root(root)
	, project(project)
	, factory(factory)
	, srcLanguage(srcLanguage)
	, dstLanguage(dstLanguage)
	, canEdit(canEdit)
{
	factory.loadUI(*this, "halley/localisation_language_editor");
}

void LocalisationLanguageEditor::onMakeUI()
{
	getWidgetAs<UIImage>("srcLanguageFlag")->setSprite(root.getFlag(srcLanguage.language));
	getWidgetAs<UILabel>("srcLanguage")->setText(root.getLanguageName(srcLanguage.language));

	getWidget("dstLanguageContainer")->setActive(dstLanguage != nullptr);
	getWidget("dstArrow")->setActive(dstLanguage != nullptr);
	if (dstLanguage) {
		getWidgetAs<UIImage>("dstLanguageFlag")->setSprite(root.getFlag(dstLanguage->language));
		getWidgetAs<UILabel>("dstLanguage")->setText(root.getLanguageName(dstLanguage->language));
	}

	setHandle(UIEventType::ButtonClicked, "close", [=] (const UIEvent& event)
	{
		root.returnToRoot();
	});
}

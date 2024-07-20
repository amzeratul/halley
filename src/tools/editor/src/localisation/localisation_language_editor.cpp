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

	Vector<UIDropdown::Entry> chunks;
	chunks.push_back(UIDropdown::Entry("", "[All]"));
	for (auto& chunk: srcLanguage.chunks) {
		String name = chunk.name;

		if (dstLanguage) {
			const auto& dstChunk = dstLanguage->getChunk(chunk.name);
			const auto srcStats = chunk.getStats();
			const auto dstStats = dstChunk.getStats();
			const int complete = srcStats.totalKeys > 0 ? std::max(dstStats.totalKeys * 100 / srcStats.totalKeys, dstStats.totalKeys > 0 ? 1 : 0) : 0;
			name = "[" + toString(complete, 10, 3, ' ') + "%] " + name;
		}

		chunks.push_back(UIDropdown::Entry(chunk.name, name));
	}
	getWidgetAs<UIDropdown>("chunk")->setOptions(chunks);

	bindData("chunk", "", [=] (String chunkId)
	{
		setChunk(chunkId);
	});

	setHandle(UIEventType::ButtonClicked, "close", [this] (const UIEvent& event)
	{
		root.returnToRoot();
	});
}

void LocalisationLanguageEditor::setChunk(const String& chunkId)
{
	// TODO
}

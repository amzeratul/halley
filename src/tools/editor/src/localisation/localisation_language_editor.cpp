#include "localisation_language_editor.h"

#include "localisation_data.h"
#include "localisation_editor_root.h"

using namespace Halley;

LocalisationLanguageEditor::LocalisationLanguageEditor(LocalisationEditorRoot& root, Project& project, UIFactory& factory, LocOriginalData& srcLanguage, LocTranslationData* dstLanguage, bool canEdit)
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
	getWidgetAs<UIImage>("srcLanguageFlag")->setSprite(root.getFlag(srcLanguage.getLanguage()));
	getWidgetAs<UILabel>("srcLanguage")->setText(root.getLanguageName(srcLanguage.getLanguage()));

	getWidget("dstLanguageContainer")->setActive(dstLanguage != nullptr);
	getWidget("dstArrow")->setActive(dstLanguage != nullptr);
	if (dstLanguage) {
		getWidgetAs<UIImage>("dstLanguageFlag")->setSprite(root.getFlag(dstLanguage->language));
		getWidgetAs<UILabel>("dstLanguage")->setText(root.getLanguageName(dstLanguage->language));
	}

	grid = std::make_shared<LocalisationGrid>(factory);
	getWidget("keysContainer")->add(grid, 1);

	Vector<UIDropdown::Entry> chunks;
	chunks.push_back(UIDropdown::Entry("", "[All]"));
	for (auto& chunk: srcLanguage.getChunks()) {
		String name = chunk.name;

		if (dstLanguage) {
			const auto srcStats = chunk.getStats();
			const auto dstStats = chunk.getStats(*dstLanguage);
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
	setChunk(chunks.empty() ? "" : chunks.front().id);

	setHandle(UIEventType::ButtonClicked, "close", [this] (const UIEvent& event)
	{
		root.returnToRoot();
	});

	setHandle(UIEventType::ListSelectionChanged, "localisation_grid", [=] (const UIEvent& event)
	{
		setSelectedLine(event.getIntData(), event.getStringData());
	});
	setSelectedLine(grid->getSelectedLine(), grid->getSelectedKey());

	setHandle(UIEventType::TextChanged, "srcCurLine", [=] (const UIEvent& event)
	{
		if (canEdit && acceptingTextInput) {
			//srcLanguage.setValue(curEditingKey, event.getStringData());
		}
	});

	setHandle(UIEventType::TextChanged, "dstCurLine", [=] (const UIEvent& event)
	{
		if (canEdit && acceptingTextInput && dstLanguage) {
			dstLanguage->setValue(curEditingKey, srcLanguage.getVersion(curEditingKey), event.getStringData());
		}
	});
}

void LocalisationLanguageEditor::setChunk(const String& chunkId)
{
	srcData = chunkId.isEmpty() ? static_cast<const ILocOriginalData*>(&srcLanguage) : srcLanguage.tryGetChunk(chunkId);
	grid->setData(srcData, dstLanguage);
}

void LocalisationLanguageEditor::setSelectedLine(int idx, const String& key)
{
	auto curKey = getWidgetAs<UILabel>("curKey");
	auto srcCurLine = getWidgetAs<UITextInput>("srcCurLine");
	auto dstCurLine = getWidgetAs<UITextInput>("dstCurLine");

	curEditingKey = key;

	acceptingTextInput = false;
	if (idx >= 0) {
		curKey->setText(LocalisedString::fromUserString(key));
		srcCurLine->setText(srcData->getEntry(idx).value);
		srcCurLine->setReadOnly(true);
		if (dstLanguage) {
			const auto* entry = dstLanguage->tryGetEntry(srcData->getEntry(idx).key);
			dstCurLine->setText(entry ? entry->value : "");
			dstCurLine->setReadOnly(!canEdit);
		}
	} else {
		curKey->setText(LocalisedString::fromHardcodedString("N/A"));
		srcCurLine->setText("");
		srcCurLine->setReadOnly(true);
		dstCurLine->setText("");
		dstCurLine->setReadOnly(true);
	}
	acceptingTextInput = true;
}

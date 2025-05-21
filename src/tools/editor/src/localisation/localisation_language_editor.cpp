#include "localisation_language_editor.h"

#include "localisation_client.h"
#include "localisation_data.h"
#include "localisation_editor_root.h"
#include "localisation_set_filters_window.h"

using namespace Halley;

LocalisationLanguageEditor::LocalisationLanguageEditor(LocalisationEditorRoot& root, LocalisationClient& client, Project& project, UIFactory& factory, LocOriginalData& srcLanguage, LocTranslationData* dstLanguage, LocOriginalData* srcRemote, LocTranslationData* locRemote, bool canEdit)
	: UIWidget("localisation_language_editor", {}, UISizer())
	, root(root)
	, client(client)
	, project(project)
	, factory(factory)
	, srcLanguage(srcLanguage)
	, dstLanguage(dstLanguage)
	, srcRemote(srcRemote)
	, locRemote(locRemote)
	, canEdit(canEdit)
{
	factory.loadUI(*this, "halley/localisation/localisation_language_editor");
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
		close();
	});

	setHandle(UIEventType::ButtonClicked, "setFilters", [this] (const UIEvent& event)
	{
		setFilters();
	});

	setHandle(UIEventType::ButtonClicked, "clearFilters", [this] (const UIEvent& event)
	{
		filters.clearFilters();
		onFiltersUpdated();
	});

	bindData("searchBar", filters.searchString, [this] (String value)
	{
		filters.searchString = std::move(value);
		onFiltersUpdated();
	});

	setHandle(UIEventType::ListSelectionChanged, "localisation_grid", [=] (const UIEvent& event)
	{
		setSelectedLine(event.getIntData(), event.getStringData());
	});
	setSelectedLine(grid->getActiveSelectedLine(), grid->getActiveSelectedKey());

	setHandle(UIEventType::TextChanged, "srcCurLine", [=] (const UIEvent& event)
	{
		setSrcValue(event.getStringData());
	});

	setHandle(UIEventType::TextChanged, "dstCurLine", [=] (const UIEvent& event)
	{
		setDstValue(event.getStringData());
	});

	setHandle(UIEventType::TextSubmit, "dstCurLine", [=] (const UIEvent& event)
	{
		// Go to next line
		grid->setSelectedLine(grid->getActiveSelectedLine() + 1);
	});

	setHandle(UIEventType::TextSubmit, "comment", [=] (const UIEvent& event)
	{
		setComment(event.getStringData());
	});

	setHandle(UIEventType::TextSubmit, "context", [=] (const UIEvent& event)
	{
		setContext(event.getStringData());
	});

	setHandle(UIEventType::DropdownSelectionChanged, "priority", [=] (const UIEvent& event)
	{
		setPriority(fromString<LocPriority>(event.getStringData()));
	});

	getWidget("editProperties")->setActive(srcRemote != nullptr);
}

void LocalisationLanguageEditor::update(Time t, bool moved)
{
	uploadPendingTranslations(false);
}

void LocalisationLanguageEditor::setChunk(const String& chunkId)
{
	srcData = chunkId.isEmpty() ? static_cast<const ILocOriginalData*>(&srcLanguage) : srcLanguage.tryGetChunk(chunkId);

	srcRemoteDataIndex.clear();
	if (srcRemote) {
		const auto n = srcRemote->getNumEntries();
		for (size_t i = 0; i < n; ++i) {
			srcRemoteDataIndex[srcRemote->getEntry(i).key] = i;
		}
	}

	grid->setLineColourFilter([this] (int idx) -> std::optional<Colour4f> {
		return getRowColour(idx);
	});
	grid->setData(srcData, dstLanguage, srcRemote != nullptr);
}

void LocalisationLanguageEditor::setSelectedLine(int idx, const String& key)
{
	const bool hasMultiSel = grid->getSelectedLines().size() > 1;

	auto curKey = getWidgetAs<UILabel>("curKey");
	auto srcCurLine = getWidgetAs<UITextInput>("srcCurLine");
	auto dstCurLine = getWidgetAs<UITextInput>("dstCurLine");
	auto comment = getWidgetAs<UITextInput>("comment");
	auto context = getWidgetAs<UITextInput>("context");
	auto priority = getWidgetAs<UIDropdown>("priority");

	curEditingKey = key;

	bool canEditProperties = canEdit && srcRemote;
	comment->setReadOnly(!canEditProperties);
	context->setReadOnly(!canEditProperties);
	priority->setEnabled(canEditProperties);

	acceptingTextInput = false;
	if (idx >= 0) {
		const auto& srcEntry = srcData->getEntry(idx);

		curKey->setText(LocalisedString::fromUserString(key));
		srcCurLine->setText(srcEntry.value);
		srcCurLine->setReadOnly(true);

		if (canEdit) {
			comment->setText(srcEntry.comment);
			context->setText(srcEntry.context);
			priority->setSelectedOption(toString(srcEntry.priority));
		}

		if (dstLanguage) {
			const auto* dstEntry = dstLanguage->tryGetEntry(srcEntry.key);
			dstCurLine->setText(dstEntry ? dstEntry->value : "");
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

void LocalisationLanguageEditor::setSrcValue(const String& value)
{
	if (canEdit && acceptingTextInput) {
		srcLanguage.setValue(curEditingKey, value);
	}
}

void LocalisationLanguageEditor::setDstValue(const String& value)
{
	if (canEdit && acceptingTextInput && dstLanguage) {
		dstLanguage->setValue(curEditingKey, srcLanguage.getVersion(curEditingKey), value);

		pendingTranslationModifiedKeys += curEditingKey;
	}
}

void LocalisationLanguageEditor::setComment(const String& comment)
{
	Vector<String> modified;
	for (const auto lineNumber: grid->getSelectedLines()) {
		const auto& key = grid->getKeyAt(lineNumber);
		if (auto* entry = srcLanguage.tryGetEntry(key)) {
			if (entry->comment != comment) {
				entry->comment = comment;
				modified += key;
			}
		}
	}
	onStringPropertiesModified(modified);
}

void LocalisationLanguageEditor::setContext(const String& context)
{
	Vector<String> modified;
	for (const auto lineNumber: grid->getSelectedLines()) {
		const auto& key = grid->getKeyAt(lineNumber);
		if (auto* entry = srcLanguage.tryGetEntry(key)) {
			if (entry->context != context) {
				entry->context = context;
				modified += key;
			}
		}
	}
	onStringPropertiesModified(modified);
}

void LocalisationLanguageEditor::setPriority(LocPriority priority)
{
	Vector<String> modified;
	for (const auto lineNumber: grid->getSelectedLines()) {
		const auto& key = grid->getKeyAt(lineNumber);
		if (auto* entry = srcLanguage.tryGetEntry(key)) {
			if (entry->priority != priority) {
				entry->priority = priority;
				modified += key;
			}
		}
	}
	onStringPropertiesModified(modified);
}

void LocalisationLanguageEditor::onStringPropertiesModified(const Vector<String>& keys)
{
	if (!srcRemote || keys.empty()) {
		return;
	}

	auto entries = srcLanguage.makeStringPropertiesDelta(*srcRemote, keys);
	if (!entries.empty()) {
		auto future = client.putStringProperties(entries); // Do not merge these two lines, note the std::move(entries) below
		future.then(aliveFlag, Executors::getMainUpdateThread(), [remote = srcRemote, entries = std::move(entries)](bool ok) {
			if (ok) {
				remote->applyStringProperties(entries);
			}
		});
	}
}

void LocalisationLanguageEditor::uploadPendingTranslations(bool force)
{
	if (!locRemote || !dstLanguage) {
		return;
	}

	if (uploadingKeys && !force) {
		return;
	}

	auto keys = std::move(pendingTranslationModifiedKeys);
	pendingTranslationModifiedKeys = {};
	auto localisedDelta = force ? dstLanguage->makeDeltaFrom(*locRemote) : dstLanguage->makeDeltaFrom(*locRemote, keys);

	if (localisedDelta.entries.empty()) {
		// Nothing to do here
		return;
	}

	uploadingKeys = true;
	auto future = client.putTranslatedStrings(localisedDelta);
	auto future2 = future.then(aliveFlag, Executors::getMainUpdateThread(), [this, keys = std::move(keys), localisedDelta = std::move(localisedDelta)](bool ok) {
		uploadingKeys = false;
		if (ok) {
			locRemote->update(localisedDelta);
		} else {
			pendingTranslationModifiedKeys += keys;
		}
	});

	if (force) {
		future2.wait();
	}
}

void LocalisationLanguageEditor::close()
{
	uploadPendingTranslations(true);
	root.returnToRoot();
}

void LocalisationLanguageEditor::setFilters()
{
	const auto pos = getWidget("setFilters")->getRect().getBottomLeft();
	getRoot()->addChild(std::make_shared<LocalisationSetFiltersWindow>(factory, filters, pos, [=] (bool changed) {
		if (changed) {
			onFiltersUpdated();
		}
	}));
}

void LocalisationLanguageEditor::onFiltersUpdated()
{
	// TODO
}

std::optional<Colour4f> LocalisationLanguageEditor::getRowColour(int idx) const
{
	if (srcRemote) {
		const auto& localEntry = srcData->getEntry(idx);
		const auto* remoteEntry = srcRemote->tryGetEntry(localEntry.key);

		if (dstLanguage) {
			// Editing translation
			const auto* localTranslation = dstLanguage->tryGetEntry(localEntry.key);
			if (remoteEntry && localTranslation) {
				if (localTranslation->origVersion < remoteEntry->version) {
					// Outdated
					return Colour4f(1.0f, 0.9f, 0.0f, 0.2f);
				}
			}
		} else {
			// Editing original language
			if (remoteEntry) {
				if (remoteEntry->value != localEntry.value) {
					// This entry is different from remote
					return Colour4f(1.0f, 0.9f, 0.0f, 0.2f);
				}
			} else {
				// This entry doesn't exist on remote
				return Colour4f(0.0f, 1.0f, 0.0f, 0.2f);
			}
		}
	}

	return {};
}

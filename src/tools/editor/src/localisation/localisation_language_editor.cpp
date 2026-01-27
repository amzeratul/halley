#include "localisation_language_editor.h"

#include "localisation_client.h"
#include "localisation_data.h"
#include "localisation_editor_root.h"
#include "localisation_set_filters_window.h"
#include "halley/tools/project/project.h"
#include "halley/tools/project/project_properties.h"

using namespace Halley;

LocalisationLanguageEditor::LocalisationLanguageEditor(LocalisationEditorRoot& root, LocalisationClient& client, Project& project, UIFactory& factory, const HalleyAPI& api, LocOriginalData& srcLanguage, LocTranslationData* dstLanguage, LocOriginalData* srcRemote, LocTranslationData* locRemote, bool canEdit)
	: UIWidget("localisation_language_editor", {}, UISizer())
	, root(root)
	, client(client)
	, project(project)
	, factory(factory)
	, api(api)
	, srcLanguage(srcLanguage)
	, dstLanguage(dstLanguage)
	, srcRemote(srcRemote)
	, locRemote(locRemote)
	, canEdit(canEdit)
{
	filterRules = project.getProperties().getLocFilterRules();
	filters.initialise(srcRemote != nullptr && dstLanguage != nullptr);

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

	grid = std::make_shared<LocalisationGrid>(factory, api, filterRules);
	getWidget("keysContainer")->add(grid, 1);

	Vector<UIDropdown::Entry> chunks;
	chunks.push_back(UIDropdown::Entry("", "[All]"));
	for (auto& chunk: srcLanguage.getChunks()) {
		String name = chunk.name;

		if (dstLanguage) {
			const auto srcStats = chunk.getStats(filterRules);
			const auto dstStats = chunk.getStats(*dstLanguage, filterRules);
			const int complete = srcStats.totalWords > 0 ? std::max(dstStats.totalWords * 100 / srcStats.totalWords, dstStats.totalWords > 0 ? 1 : 0) : 0;
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

	setHandle(UIEventType::ButtonClicked, "clearSearch", [this] (const UIEvent& event)
	{
		getWidgetAs<UITextInput>("searchBar")->setText(StringUTF32());
	});

	bindData("searchBar", filters.searchString, [this] (String value)
	{
		filters.searchString = value.asciiLower();
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
		grid->moveSelection(1);
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

	setHandle(UIEventType::ButtonClicked, "markUpToDate", [this] (const UIEvent& event)
	{
		markUpToDate();
	});

	setHandle(UIEventType::ButtonClicked, "markOutOfDate", [this] (const UIEvent& event)
	{
		markOutOfDate();
	});

	getWidget("editProperties")->setActive(srcRemote != nullptr);
	const auto canEditTranslation = canEdit && srcRemote != nullptr && dstLanguage != nullptr;
	getWidget("markUpToDate")->setActive(canEditTranslation);
	getWidget("markOutOfDate")->setActive(canEditTranslation);

	onFiltersUpdated();
}

void LocalisationLanguageEditor::update(Time t, bool moved)
{
	getWidget("clearSearch")->setEnabled(!filters.searchString.isEmpty());

	const auto canEditTranslation = canEdit && srcRemote != nullptr && dstLanguage != nullptr;
	if (canEditTranslation) {
		const auto [outOfDate, upToDate] = getOutOfDateAndUpToDateCountInSelection();
		getWidget("markUpToDate")->setEnabled(outOfDate > 0);
		getWidget("markOutOfDate")->setEnabled(upToDate > 0);
	}

	uploadPendingTranslations(false);
}

void LocalisationLanguageEditor::onStringsUpdated()
{
	setChunk(curChunk);
}

void LocalisationLanguageEditor::setChunk(const String& chunkId)
{
	curChunk = chunkId;
	srcData = chunkId.isEmpty() ? static_cast<const ILocOriginalData*>(&srcLanguage) : srcLanguage.tryGetChunk(chunkId);

	srcRemoteDataIndex.clear();
	if (srcRemote) {
		const auto n = srcRemote->getNumEntries();
		for (size_t i = 0; i < n; ++i) {
			srcRemoteDataIndex[srcRemote->getEntry(i).getKey()] = i;
		}
	}

	grid->setLineColourFilter([this] (int idx) -> std::optional<Colour4f> {
		return getRowColour(idx);
	});
	grid->setFilter([this] (int idx) -> bool {
		return isRowVisible(idx);
	}, false);
	grid->setData(srcData, dstLanguage, srcRemote != nullptr);

	applyFilters();
}

void LocalisationLanguageEditor::setSelectedLine(int idx, const String& key)
{
	auto curKey = getWidgetAs<UILabel>("curKey");
	auto srcCurLine = getWidgetAs<UITextInput>("srcCurLine");
	auto dstCurLine = getWidgetAs<UITextInput>("dstCurLine");
	auto comment = getWidgetAs<UITextInput>("comment");
	auto context = getWidgetAs<UITextInput>("context");
	auto priority = getWidgetAs<UIDropdown>("priority");

	curEditingKey = key;

	bool canEditProperties = canEditPropertiesForSelection();
	comment->setReadOnly(!canEditProperties);
	context->setReadOnly(!canEditProperties);
	priority->setEnabled(canEditProperties);

	acceptingTextInput = false;
	if (idx >= 0) {
		const auto& srcEntry = srcData->getEntry(idx);

		curKey->setText(LocalisedString::fromUserString(key));
		srcCurLine->setText(srcEntry.getValue());
		srcCurLine->setReadOnly(true);

		if (canEdit) {
			comment->setText(srcEntry.getComment());
			context->setText(srcEntry.getContext());
			priority->setSelectedOption(toString(srcEntry.getPriority()));
		}

		if (dstLanguage) {
			const auto* dstEntry = dstLanguage->tryGetEntry(srcEntry.getKey());
			dstCurLine->setText(dstEntry ? dstEntry->getValue() : "");
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
	grid->refreshContents();
	root.getRemoteClientUpdater().onStringUpdated(curEditingKey);
}

void LocalisationLanguageEditor::setDstValue(const String& value)
{
	if (canEdit && acceptingTextInput && dstLanguage) {
		dstLanguage->setValue(curEditingKey, srcLanguage.getVersion(curEditingKey), value);

		if (!pendingTranslationTextEditedKeys.contains(curEditingKey)) {
			pendingTranslationTextEditedKeys += curEditingKey;
		}
	}
	grid->refreshContents();
	root.getRemoteClientUpdater().onStringUpdated(curEditingKey);
}

void LocalisationLanguageEditor::setComment(const String& comment)
{
	if (!canEditPropertiesForSelection()) {
		return;
	}

	Vector<String> modified;
	for (const auto lineNumber: grid->getSelectedLines()) {
		const auto& key = grid->getKeyAt(lineNumber);
		if (auto* entry = srcLanguage.tryGetEntry(key)) {
			if (entry->getComment() != comment) {
				entry->setComment(comment);
				modified += key;
			}
		}
	}
	onStringPropertiesModified(modified);
	grid->refreshContents();
}

void LocalisationLanguageEditor::setContext(const String& context)
{
	if (!canEditPropertiesForSelection()) {
		return;
	}

	Vector<String> modified;
	for (const auto lineNumber: grid->getSelectedLines()) {
		const auto& key = grid->getKeyAt(lineNumber);
		if (auto* entry = srcLanguage.tryGetEntry(key)) {
			if (entry->getContext() != context) {
				entry->setContext(context);
				modified += key;
			}
		}
	}
	onStringPropertiesModified(modified);
	grid->refreshContents();
}

void LocalisationLanguageEditor::setPriority(LocPriority priority)
{
	if (!canEditPropertiesForSelection()) {
		return;
	}

	Vector<String> modified;
	for (const auto lineNumber: grid->getSelectedLines()) {
		const auto& key = grid->getKeyAt(lineNumber);
		if (auto* entry = srcLanguage.tryGetEntry(key)) {
			if (entry->getPriority() != priority) {
				entry->setPriority(priority);
				modified += key;
			}
		}
	}
	onStringPropertiesModified(modified);
	grid->refreshContents();
}

bool LocalisationLanguageEditor::canEditProperties(int idx) const
{
	if (!canEdit || !srcRemote) {
		return false;
	}

	const auto& localEntry = srcData->getEntry(idx);
	const auto* remoteEntry = srcRemote->tryGetEntry(localEntry.getKey());
	if (!remoteEntry) {
		return false;
	}

	return true;
}

bool LocalisationLanguageEditor::canEditPropertiesForSelection() const
{
	if (!canEdit || !srcRemote) {
		return false;
	}

	const auto& sel = grid->getSelectedLines();
	if (sel.empty()) {
		return false;
	}

	for (const auto lineNumber : sel) {
		if (!canEditProperties(lineNumber)) {
			return false;
		}
	}
	return true;
}

void LocalisationLanguageEditor::markUpToDate()
{
	if (!srcRemote || !dstLanguage) {
		return;
	}

	for (const auto lineNumber: grid->getSelectedLines()) {
		const auto& key = grid->getKeyAt(lineNumber);
		if (auto* srcEntry = srcLanguage.tryGetEntry(key)) {
			if (srcEntry->getVersion() >= 0) {
				if (dstLanguage->setVersion(key, srcEntry->getVersion())) {
					pendingTranslationModifiedKeys += key;
				}
			}
		}
	}
	grid->refreshContents();
}

void LocalisationLanguageEditor::markOutOfDate()
{
	if (!srcRemote || !dstLanguage) {
		return;
	}

	for (const auto lineNumber: grid->getSelectedLines()) {
		const auto& key = grid->getKeyAt(lineNumber);
		if (auto* srcEntry = srcLanguage.tryGetEntry(key)) {
			if (srcEntry->getVersion() > 0) {
				if (dstLanguage->setVersion(key, srcEntry->getVersion() - 1)) {
					pendingTranslationModifiedKeys += key;
				}
			}
		}
	}
	grid->refreshContents();
}

std::pair<int, int> LocalisationLanguageEditor::getOutOfDateAndUpToDateCountInSelection() const
{
	if (!srcRemote || !dstLanguage) {
		return {};
	}
	
	int upToDate = 0;
	int outOfDate = 0;

	for (const auto lineNumber: grid->getSelectedLines()) {
		const auto& key = grid->getKeyAt(lineNumber);
		if (auto* srcEntry = srcRemote->tryGetEntry(key)) {
			if (auto* dstEntry = dstLanguage->tryGetEntry(key)) {
				if (dstEntry->origVersion != srcEntry->getVersion()) {
					++outOfDate;
				} else {
					++upToDate;
				}
			}
		}
	}
	return { outOfDate, upToDate };
}

void LocalisationLanguageEditor::onStringPropertiesModified(const Vector<String>& keys)
{
	if (!srcRemote || keys.empty()) {
		return;
	}

	auto entries = srcLanguage.makeStringPropertiesDelta(*srcRemote, keys);
	if (!entries.empty()) {
		Logger::logDev(toString(keys.size()) + " keys marked as modified properties, sending delta for " + toString(entries.size()));
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

	// Editing text keys, if no longer active (or forcing an upload), move it to pending changes list
	Vector<String> toRemoveEditing;
	for (auto& key: pendingTranslationTextEditedKeys) {
		if (key != curEditingKey || force) {
			toRemoveEditing += key;
			pendingTranslationModifiedKeys += key;
		}
	}
	std_ex::erase_if(pendingTranslationTextEditedKeys, [&] (const auto& key) { return toRemoveEditing.contains(key); });

	// Create a list of actual changes to upload
	auto keys = std::move(pendingTranslationModifiedKeys);
	pendingTranslationModifiedKeys = {};
	auto localisedDelta = force ? dstLanguage->makeDeltaFrom(*locRemote) : dstLanguage->makeDeltaFrom(*locRemote, keys);

	if (localisedDelta.entries.empty()) {
		// Nothing to do here
		return;
	}

	uploadingKeys = true;
	auto future = client.putTranslatedStrings(localisedDelta);
	auto future2 = future.then(aliveFlag, force ? Executors::getCPU() : Executors::getMainUpdateThread(), [this, keys = std::move(keys), localisedDelta = std::move(localisedDelta)](bool ok) {
		uploadingKeys = false;
		if (ok) {
			//Logger::logInfo("Uploaded " + toString(keys.size()) + " entries.");
			locRemote->update(localisedDelta);
		} else {
			Logger::logWarning("Failed to upload " + toString(keys.size()) + " modified entries.");
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
	getWidget("clearFilters")->setEnabled(filters.hasFiltersActive());

	updateFilterDisplay();
	applyFilters();
}

void LocalisationLanguageEditor::updateFilterDisplay()
{
	getWidgetAs<UILabel>("filtersLabel")->setText(LocalisedString::fromUserString(filters.toString()));
}

void LocalisationLanguageEditor::applyFilters()
{
	grid->refreshFilter();

	const auto activeRows = grid->getActiveRowCount();
	const auto srcRows = grid->getSrcRowCount();

	int wordCount = 0;
	for (const auto idx: grid->getActiveRows()) {
		wordCount += LocalisationStats::getWordCount(srcData->getEntry(idx).getValue());
	}

	const auto origLang = project.getProperties().getOriginalLanguage();
	String showingString;
	if (activeRows < srcRows) {
		showingString = "Showing " + toString(activeRows) + " out of " + toString(srcRows) + " rows";
	} else {
		showingString = "Showing all " + toString(srcRows) + " rows";
	}
	showingString += " (" + toString(wordCount) + " " + root.getLanguageName(origLang, false) + " words)";

	getWidgetAs<UILabel>("showingLabel")->setText(LocalisedString::fromUserString(showingString));
}

std::optional<Colour4f> LocalisationLanguageEditor::getRowColour(int idx) const
{
	if (srcRemote) {
		const auto& localEntry = srcData->getEntry(idx);
		const auto* remoteEntry = srcRemote->tryGetEntry(localEntry.getKey());

		if (dstLanguage) {
			// Editing translation
			const auto* localTranslation = dstLanguage->tryGetEntry(localEntry.getKey());
			if (remoteEntry && localTranslation) {
				if (localTranslation->origVersion < remoteEntry->getVersion()) {
					// Outdated
					return Colour4f(1.0f, 0.9f, 0.0f, 0.2f);
				}
			}
		} else {
			// Editing original language
			if (remoteEntry) {
				if (remoteEntry->getValue() != localEntry.getValue()) {
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

bool LocalisationLanguageEditor::isRowVisible(int idx) const
{
	const auto& original = srcData->getEntry(idx);
	const auto* translated = dstLanguage ? dstLanguage->tryGetEntry(original.getKey()) : nullptr;

	return filters.shouldShow(original, translated, filterRules);
}

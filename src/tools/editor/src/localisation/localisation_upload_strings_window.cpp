#include "localisation_upload_strings_window.h"

#include "localisation_client.h"
#include "localisation_grid.h"
#include "src/ui/project_window.h"

using namespace Halley;

LocUploadStringsGrid::LocUploadStringsGrid(UIFactory& factory, LocStringUploadData& data, HashMap<String, Vector<String>>& keysLocalisedIn)
	: UIGrid("grid", factory)
	, uploadData(data)
	, keysLocalisedIn(keysLocalisedIn)
{
	uploadData.makeDiff();

	tickSprite = Sprite().setImage(factory.getResources(), "ui/check.png");
	locSprite = Sprite().setImage(factory.getResources(), "ui/localised.png");
	minorRevSprite = Sprite().setImage(factory.getResources(), "ui/spellcheck.png");

	setLineColourFilter([=] (int row) {
		return getRowColour(row);
	});

	generateMapping();
	onDataUpdated();
}

const String& LocUploadStringsGrid::getKeyAt(int idx) const
{
	return getEntry(idx).key;
}

size_t LocUploadStringsGrid::getSrcRowCount() const
{
	return mapping.size();
}

std::pair<Vector<float>, Vector<String>> LocUploadStringsGrid::getColumns() const
{
	const float width = getSize().x - 1;
	const float fixedWidth = 25 + 25 + 25 + 120 + 40;
	const float remainingWidth = width - fixedWidth;

	Vector<float> sizes = { 25.0f, 25.0f, 25.0f, 120.0f, 40.0f, remainingWidth * 0.20f, remainingWidth * 0.8f };
	Vector<String> names { "Send", "Loc", "Min", "Group", "Status", "Key", "Diff" };
	return { sizes, names };
}

void LocUploadStringsGrid::getLineDrawData(int idx, Vector<String>& strs, Vector<Colour4f>& colours, Vector<Sprite>& sprites, Vector<Vector<ColourOverride>>& colourOverrides) const
{
	const auto& e = getEntry(idx);

	int nSprites = 3;
	int len = 5 + nSprites;

	strs.resize(len);
	strs[nSprites + 0] = Path(getChunk(idx).chunkId).getFilename();
	strs[nSprites + 1] = getTypeDesc(e.type, e.minorRevision);
	strs[nSprites + 2] = (e.oldKey ? "*" : "") + e.key;
	//strs[nSprites + 3] = e.remoteValue.value_or("");
	//strs[nSprites + 4] = e.value;
	strs[nSprites + 3] = e.valueDiff ? e.valueDiff->str : "";

	colours.resize(len, textCol);
	
	colourOverrides.resize(len, {});
	colourOverrides[nSprites + 3] = e.valueDiff ? getColourOverrides(e.valueDiff->changeTypes) : Vector<ColourOverride>();

	sprites.resize(len, {});
	sprites[0] = e.send ? tickSprite : Sprite();
	sprites[1] = keysLocalisedIn.contains(e.oldKey.value_or(e.key)) ? locSprite : Sprite();
	sprites[2] = e.minorRevision ? minorRevSprite : Sprite();
}

String LocUploadStringsGrid::getCellToolTip(int row, int col, const String& columnName) const
{
	if (columnName == "Group") {
		const auto& c = getChunk(row);
		return c.chunkId;
	} else {
		const auto& e = getEntry(row);
		if (columnName == "Send") {
			return e.send ? "Sending" : "Not sending";
		} else if (columnName == "Loc") {
			if (const auto iter = keysLocalisedIn.find(e.key); iter != keysLocalisedIn.end()) {
				return "Localised in: " + String::concatList(iter->second, ", ");
			} else {
				return "Not localised";
			}
		} else if (columnName == "Min") {
			return e.minorRevision ? "Minor revision" : "";
		} else if (columnName == "Status") {
			return "Status: " + toString(e.type);
		} else if (columnName == "Key") {
			return (e.oldKey ? *e.oldKey + "\n->\n" : "") + e.key;
		} else if (columnName == "Diff") {
			return (e.remoteValue ? *e.remoteValue + "\n->\n" : "") + e.value;
		}
	}
	return {};
}

void LocUploadStringsGrid::onRightClick(std::optional<int> line)
{
}

void LocUploadStringsGrid::copySelection()
{
}

void LocUploadStringsGrid::generateMapping()
{
	mapping.clear();
	for (size_t i = 0; i < uploadData.getChunks().size(); ++i) {
		const auto& chunk = uploadData.getChunks()[i];
		for (size_t j = 0; j < chunk.entries.size(); ++j) {
			const auto& entry = chunk.entries[j];
			if (entry.type != LocStringUploadEntryType::Noop) {
				mapping += { static_cast<int>(i), static_cast<int>(j) };
			}
		}
	}
}

const LocStringUploadChunkData& LocUploadStringsGrid::getChunk(int idx) const
{
	const auto i = mapping[idx];
	return uploadData.getChunks()[i.first];	
}

LocStringUploadChunkData::Entry& LocUploadStringsGrid::getEntry(int idx) const
{
	const auto i = mapping[idx];
	return uploadData.getChunks()[i.first].entries[i.second];
}

size_t LocUploadStringsGrid::getNumEntries() const
{
	return mapping.size();
}

String LocUploadStringsGrid::getTypeDesc(LocStringUploadEntryType type, bool minor) const
{
	switch (type) {
	case LocStringUploadEntryType::Added:
		return "ADD";
	case LocStringUploadEntryType::Renamed:
		return "REN";
	case LocStringUploadEntryType::Modified:
		return minor ? "MIN" : "MOD";
	case LocStringUploadEntryType::Removed:
		return "DEL";
	case LocStringUploadEntryType::Noop:
		return "NOP";
	}
	return "???";
}

std::optional<Colour4f> LocUploadStringsGrid::getRowColour(int row) const
{
	constexpr float alpha = 0.2f;

	const auto& e = getEntry(row);
	switch (e.type) {
	case LocStringUploadEntryType::Added:
		return Colour4f(0.2f, 1.0f, 0.2f, alpha);
	case LocStringUploadEntryType::Renamed:
		return Colour4f(0.2f, 0.2f, 1.0f, alpha);
	case LocStringUploadEntryType::Removed:
		return Colour4f(1.0f, 0.2f, 0.2f, alpha);
	case LocStringUploadEntryType::Modified:
		return e.minorRevision ? Colour4f(0.2f, 1.0f, 1.0f, alpha) : Colour4f(1.0f, 1.0f, 0.2f, alpha);
	}
	return {};
}

Vector<ColourOverride> LocUploadStringsGrid::getColourOverrides(const Vector<std::pair<StringDiffType, size_t>>& values) const
{
	Vector<ColourOverride> result;
	result.resize(values.size());

	for (size_t i = 0; i < values.size(); ++i) {
		result[i] = ColourOverride(values[i].second, getColourOverride(values[i].first));
	}

	return result;
}

Colour4f LocUploadStringsGrid::getColourOverride(StringDiffType diffType) const
{
	switch (diffType) {
	case StringDiffType::Add:
		return Colour4f(0.1f, 1, 0.15f, 1);
	case StringDiffType::Delete:
		return Colour4f(1.0f, 0, 0, 1);
	case StringDiffType::Common:
	default:
		return Colour4f(1, 1, 1, 1);
	}
}

LocUploadStringsWindow::LocUploadStringsWindow(UIFactory& factory, ProjectWindow& projectWindow, LocalisationClient& client, LocStringUploadData uploadData, HashMap<String, Vector<String>> keysLocalisedIn)
	: UIWidget("upload_strings", {}, UISizer())
	, factory(factory)
	, projectWindow(projectWindow)
	, client(client)
	, uploadData(std::move(uploadData))
	, keysLocalisedIn(std::move(keysLocalisedIn))
	, testMode(false)
{
	setAnchor(UIAnchor());
	factory.loadUI(*this, "halley/localisation/localisation_upload_strings");
}

void LocUploadStringsWindow::onMakeUI()
{
	setStatus("Idle", Status::Idle);

	if (testMode) {
		getWidgetAs<UIButton>("upload")->setLabel(LocalisedString::fromHardcodedString("Test Upload"));
	}

	grid = std::make_shared<LocUploadStringsGrid>(factory, uploadData, keysLocalisedIn);
	loadState();
	grid->setFilter([this] (int row) -> bool {
		if (onlyShowSend || onlyShowModified) {
			const auto& entry = grid->getEntry(row);
			if (onlyShowSend && !entry.send) {
				return false;
			}
			if (onlyShowModified && entry.type != LocStringUploadEntryType::Modified) {
				return false;
			}
		}
		return true;
	});
	getWidget("gridContainer")->add(grid, 1);

	setHandle(UIEventType::ButtonClicked, "upload", [this] (const UIEvent& event) {
		upload();
	});
	
	setHandle(UIEventType::ButtonClicked, "cancel", [this] (const UIEvent& event) {
		destroy();
	});
	
	setHandle(UIEventType::ButtonClicked, "markSend", [this] (const UIEvent& event) {
		markSend(true);
	});
	
	setHandle(UIEventType::ButtonClicked, "unmarkSend", [this] (const UIEvent& event) {
		markSend(false);
	});
	
	setHandle(UIEventType::ButtonClicked, "markMinor", [this] (const UIEvent& event) {
		markMinor(true);
	});
	
	setHandle(UIEventType::ButtonClicked, "unmarkMinor", [this] (const UIEvent& event) {
		markMinor(false);
	});
	
	setHandle(UIEventType::ButtonClicked, "selectGroup", [this] (const UIEvent& event) {
		selectGroup();
	});
	
	setHandle(UIEventType::ButtonClicked, "saveReport", [this] (const UIEvent& event) {
		saveReport();
	});

	bindData("onlyShowSend", onlyShowSend, [this] (bool value) {
		onlyShowSend = value;
		grid->refreshFilter();
	});

	bindData("onlyShowModified", onlyShowModified, [this] (bool value) {
		onlyShowModified = value;
		grid->refreshFilter();
	});

	updateSummary();
}

void LocUploadStringsWindow::onAddedToRoot(UIRoot& root)
{
	setMinSize(root.getRect().getSize() * 0.85f);
}

void LocUploadStringsWindow::update(Time t, bool moved)
{
	setMinSize(getRoot()->getRect().getSize() * 0.85f);
}

void LocUploadStringsWindow::upload()
{
	int count = 0;
	for (const auto& c: uploadData.getChunks()) {
		for (const auto& e: c.entries) {
			if (e.send && e.type != LocStringUploadEntryType::Noop) {
				++count;
			}
		}
	}

	if (count == 0) {
		return;
	}

	const auto buttons = Vector<UIConfirmationPopup::ButtonType>{ { UIConfirmationPopup::ButtonType::Yes, UIConfirmationPopup::ButtonType::Cancel }};
	getRoot()->addChild(std::make_shared<UIConfirmationPopup>(factory, "Upload Strings?", "Are you sure you want to upload these " + toString(count) + " strings?", buttons, [=](UIConfirmationPopup::ButtonType result)
	{
		if (result == UIConfirmationPopup::ButtonType::Yes) {
			doUpload();
		}
	}));
}

void LocUploadStringsWindow::doUpload()
{
	setStatus("Uploading", Status::Uploading);

	client.putOriginalStrings(uploadData, testMode).then(aliveFlag, Executors::getMainUpdateThread(), [this] (bool result)
	{
		if (result) {
			setStatus("Upload complete", Status::Success);
		} else {
			setStatus("Error uploading strings", Status::Error);
		}
	});
}

void LocUploadStringsWindow::setStatus(const String& message, Status status)
{
	curStatus = status;
	updateButtons();

	if (status == Status::Success) {
		getWidgetAs<UIButton>("cancel")->setLabel(LocalisedString::fromHardcodedString("Close"));
		saveReport();
	}
	
	auto statusLabel = getWidgetAs<UILabel>("status");
	statusLabel->setText(LocalisedString::fromUserString("Status: " + message));
	statusLabel->setColour(status == Status::Error ? Colour4f::fromHexString("#ff7e7e") : Colour4f::fromHexString("#9d9d9d"));
}

void LocUploadStringsWindow::updateSummary()
{
	auto label = getWidgetAs<UILabel>("summary");

	sendCount = 0;
	HashMap<LocStringUploadEntryType, int> counts;
	for (const auto& c: uploadData.getChunks()) {
		for (const auto& e: c.entries) {
			if (e.send && e.type != LocStringUploadEntryType::Noop) {
				++counts[e.type];
				++sendCount;
			}
		}
	}

	using enum LocStringUploadEntryType;
	label->setText(LocalisedString::fromUserString(String("Summary: ") 
		+ sendCount + " lines to send ("
		+ counts[Added] + " added, "
		+ counts[Modified] + " modified, "
		+ counts[Removed] + " removed, "
		+ counts[Renamed] + " renamed)"
	));

	updateButtons();
}

void LocUploadStringsWindow::updateButtons()
{
	getWidget("upload")->setEnabled(sendCount > 0 && curStatus != Status::Uploading && curStatus != Status::Success);
	getWidget("cancel")->setEnabled(curStatus != Status::Uploading);
	getWidget("markSend")->setEnabled(curStatus != Status::Success);
	getWidget("unmarkSend")->setEnabled(curStatus != Status::Success);
}

void LocUploadStringsWindow::markSend(bool toSend)
{
	markSend(grid->getSelectedLines(), toSend);
}

void LocUploadStringsWindow::markMinor(bool minor)
{
	markMinor(grid->getSelectedLines(), minor);
}

void LocUploadStringsWindow::markAllSend(bool toSend)
{
	HashSet<int> lines;
	const auto n = grid->getSrcRowCount();
	for (int i = 0; i < n; ++i) {
		lines.insert(i);
	}
	markSend(lines, toSend);
}

void LocUploadStringsWindow::markSend(const HashSet<int>& lines, bool toSend)
{
	for (auto line: lines) {
		grid->getEntry(line).send = toSend;
	}
	updateSummary();
	grid->refreshFilter();
	saveState();
}

void LocUploadStringsWindow::markMinor(const HashSet<int>& lines, bool minor)
{
	for (auto line: lines) {
		auto& e = grid->getEntry(line);
		if (e.type == LocStringUploadEntryType::Modified) {
			e.minorRevision = minor;
		}
	}
	grid->refreshColours();
	saveState();
}

void LocUploadStringsWindow::selectGroup()
{
	int n = grid->getActiveSelectedLine();
	if (n >= 0) {
		selectGroup(grid->getChunk(n).chunkId);
	}
}

void LocUploadStringsWindow::selectGroup(const String& id)
{
	HashSet<int> sel;
	const auto n = grid->getSrcRowCount();
	for (int i = 0; i < n; ++i) {
		if (grid->getChunk(i).chunkId == id) {
			sel.insert(i);
		}
	}
	grid->setSelectedLines(sel);
}

void LocUploadStringsWindow::saveReport()
{
	FileChooserParameters fileChooserParams;
	fileChooserParams.fileName = "exported_strings.csv";
	fileChooserParams.fileTypes.emplace_back(FileChooserParameters::FileType{ "Comma-Separated Values", {"csv"}, true });
	fileChooserParams.save = true;

	OS::get().openFileChooser(fileChooserParams).then(Executors::getMainUpdateThread(), [this](std::optional<Path> path) {
		if (path) {
			Path::writeFile(*path, generateReport());
		}
	});
}

String LocUploadStringsWindow::generateReport() const
{
	CSVFile csv;
	csv.setColumns({{ "group", "status", "oldKey", "key", "oldValue", "newValue", "translatedIn" }});

	const auto groupIdx = csv.getColumnIndex("group");
	const auto statusIdx = csv.getColumnIndex("status");
	const auto oldKeyIdx = csv.getColumnIndex("oldKey");
	const auto keyIdx = csv.getColumnIndex("key");
	const auto oldValueIdx = csv.getColumnIndex("oldValue");
	const auto newValueIdx = csv.getColumnIndex("newValue");
	const auto translatedInIdx = csv.getColumnIndex("translatedIn");

	for (const auto& chunk: uploadData.getChunks()) {
		for (const auto& entry: chunk.entries) {
			if (entry.send && entry.type != LocStringUploadEntryType::Noop) {
				const auto rowIdx = csv.addRow();
				
				csv.setCell(rowIdx, groupIdx, chunk.chunkId);
				csv.setCell(rowIdx, statusIdx, toString(entry.type));
				csv.setCell(rowIdx, oldKeyIdx, entry.oldKey.value_or(""));
				csv.setCell(rowIdx, keyIdx, entry.key);
				csv.setCell(rowIdx, oldValueIdx, entry.remoteValue.value_or(""));
				csv.setCell(rowIdx, newValueIdx, entry.value);
				csv.setCell(rowIdx, translatedInIdx, String::concatList(keysLocalisedIn.value_or(entry.key, {}).const_span(), " "));
			}
		}
	}

	return csv.save();
}

void LocUploadStringsWindow::saveState()
{
	const auto n = static_cast<int>(grid->getNumEntries());
	for (int i = 0; i < n; ++i) {
		const auto& e = grid->getEntry(i);
		if (e.send || e.minorRevision) {
			state.get(e.key) = LocUploadStringsState::Entry(e.send, e.minorRevision);
		} else {
			state.remove(e.key);
		}
	}

	projectWindow.setSetting(EditorSettingType::Project, "locUploadState", state.toConfigNode());
}

void LocUploadStringsWindow::loadState()
{
	state = LocUploadStringsState(projectWindow.getSetting(EditorSettingType::Project, "locUploadState"));

	const auto n = static_cast<int>(grid->getNumEntries());
	for (int i = 0; i < n; ++i) {
		auto& e = grid->getEntry(i);
		if (auto* stateEntry = state.tryGet(e.key)) {
			e.minorRevision = stateEntry->minor;
			e.send = stateEntry->send;
		} else {
			e.minorRevision = false;
			e.send = false;
		}
	}

	updateSummary();
	grid->refreshFilter();
	grid->refreshColours();
}

LocUploadStringsState::Entry::Entry(bool send, bool minor)
	: send(send)
	, minor(minor)
{
}

LocUploadStringsState::Entry::Entry(const ConfigNode& node)
{
	send = node["send"].asBool(false);
	minor = node["minor"].asBool(false);
}

ConfigNode LocUploadStringsState::Entry::toConfigNode() const
{
	ConfigNode result;
	result["send"] = send;
	result["minor"] = minor;
	return result;
}

LocUploadStringsState::LocUploadStringsState(const ConfigNode& node)
{
	entries = node["entries"].asHashMap<String, Entry>();
}

ConfigNode LocUploadStringsState::toConfigNode() const
{
	ConfigNode result;
	result["entries"] = entries;
	return result;
}

HashMap<String, LocUploadStringsState::Entry>& LocUploadStringsState::getEntries()
{
	return entries;
}

const HashMap<String, LocUploadStringsState::Entry>& LocUploadStringsState::getEntries() const
{
	return entries;
}

LocUploadStringsState::Entry& LocUploadStringsState::get(const String& key)
{
	return entries[key];
}

const LocUploadStringsState::Entry* LocUploadStringsState::tryGet(const String& key) const
{
	const auto iter = entries.find(key);
	if (iter != entries.end()) {
		return &iter->second;
	}
	return nullptr;
}

void LocUploadStringsState::remove(const String& key)
{
	entries.erase(key);
}

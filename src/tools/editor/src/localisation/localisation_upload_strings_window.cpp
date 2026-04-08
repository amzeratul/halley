#include "localisation_upload_strings_window.h"

#include "localisation_client.h"
#include "localisation_grid.h"

using namespace Halley;

LocUploadStringsGrid::LocUploadStringsGrid(UIFactory& factory, LocStringUploadData& data, HashMap<String, Vector<String>>& keysLocalisedIn)
	: UIGrid("grid", factory)
	, uploadData(data)
	, keysLocalisedIn(keysLocalisedIn)
{
	tickSprite = Sprite().setImage(factory.getResources(), "ui/check.png");
	locSprite = Sprite().setImage(factory.getResources(), "ui/localised.png");

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
	const float fixedWidth = 25 + 25 + 120 + 40;
	const float remainingWidth = width - fixedWidth;

	Vector<float> sizes = { 25.0f, 25.0f, 120.0f, 40.0f, remainingWidth * 0.20f, remainingWidth * 0.4f, remainingWidth * 0.4f };
	Vector<String> names { "Send", "Loc", "Group", "Status", "Key", "Previous Value", "New Value" };
	return { sizes, names };
}

void LocUploadStringsGrid::getLineDrawData(int idx, Vector<String>& strs, Vector<Colour4f>& colours, Vector<Sprite>& sprites) const
{
	const auto& e = getEntry(idx);

	int len = 7;

	strs.resize(len);
	strs[2] = Path(getChunk(idx).chunkId).getFilenameStr();
	strs[3] = getTypeDesc(e.type);
	strs[4] = (e.oldKey ? "*" : "") + e.key;
	strs[5] = e.remoteValue.value_or("");
	strs[6] = e.value;

	colours.resize(len, textCol);

	sprites.resize(len, {});
	sprites[0] = e.send ? tickSprite : Sprite();
	sprites[1] = keysLocalisedIn.contains(e.oldKey.value_or(e.key)) ? locSprite : Sprite();
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
		} else if (columnName == "Status") {
			return "Status: " + toString(e.type);
		} else if (columnName == "Key") {
			return (e.oldKey ? *e.oldKey + "\n->\n" : "") + e.key;
		} else if (columnName == "Previous Value") {
			return e.remoteValue.value_or("");
		} else if (columnName == "New Value") {
			return e.value;
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

String LocUploadStringsGrid::getTypeDesc(LocStringUploadEntryType type) const
{
	switch (type) {
	case LocStringUploadEntryType::Added:
		return "ADD";
	case LocStringUploadEntryType::Renamed:
		return "REN";
	case LocStringUploadEntryType::Modified:
		return "MOD";
	case LocStringUploadEntryType::Removed:
		return "DEL";
	case LocStringUploadEntryType::Noop:
		return "NOP";
	}
	return "???";
}

std::optional<Colour4f> LocUploadStringsGrid::getRowColour(int row) const
{
	const auto& e = getEntry(row);
	switch (e.type) {
	case LocStringUploadEntryType::Added:
		return Colour4f(0.2f, 1.0f, 0.2f, 0.1f);
	case LocStringUploadEntryType::Renamed:
		return Colour4f(0.2f, 0.2f, 1.0f, 0.1f);
	case LocStringUploadEntryType::Removed:
		return Colour4f(1.0f, 0.2f, 0.2f, 0.1f);
	case LocStringUploadEntryType::Modified:
		return Colour4f(1.0f, 1.0f, 0.2f, 0.1f);
	}
	return {};
}

LocUploadStringsWindow::LocUploadStringsWindow(UIFactory& factory, LocalisationClient& client, LocStringUploadData uploadData, HashMap<String, Vector<String>> keysLocalisedIn)
	: UIWidget("upload_strings", {}, UISizer())
	, factory(factory)
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
	markAllSend(false);
	grid->setFilter([=] (int row) -> bool {
		if (onlyShowSend) {
			return grid->getEntry(row).send;
		} else {
			return true;
		}
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
	
	setHandle(UIEventType::ButtonClicked, "selectGroup", [this] (const UIEvent& event) {
		selectGroup();
	});
	
	setHandle(UIEventType::ButtonClicked, "saveReport", [this] (const UIEvent& event) {
		saveReport();
	});

	bindData("onlyShowSend", onlyShowSend, [=] (bool value) {
		onlyShowSend = value;
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

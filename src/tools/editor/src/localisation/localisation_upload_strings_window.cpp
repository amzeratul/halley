#include "localisation_upload_strings_window.h"

#include "localisation_client.h"
#include "localisation_grid.h"

using namespace Halley;

LocUploadStringsGrid::LocUploadStringsGrid(UIFactory& factory, LocStringUploadData& data)
	: UIGrid("grid", factory)
	, uploadData(data)
{
	tickSprite = Sprite().setImage(factory.getResources(), "ui/check.png");
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
	const float fixedWidth = 20 + 120 + 40;
	const float remainingWidth = width - fixedWidth;

	Vector<float> sizes = { 20.0f, 120.0f, 40.0f, remainingWidth * 0.20f, remainingWidth * 0.4f, remainingWidth * 0.4f };
	Vector<String> names { "Send", "Group", "Status", "Key", "Previous Value", "New Value" };
	return { sizes, names };
}

void LocUploadStringsGrid::getLineDrawData(int idx, Vector<String>& strs, Vector<Colour4f>& colours, Vector<Sprite>& sprites) const
{
	const auto& e = getEntry(idx);

	int len = 6;

	strs.resize(len);
	strs[1] = Path(getChunk(idx).chunkId).getFilenameStr();
	strs[2] = getTypeDesc(e.type);
	strs[3] = (e.oldKey ? "*" : "") + e.key;
	strs[4] = e.remoteValue.value_or("");
	strs[5] = e.value;

	colours.resize(len, textCol);

	sprites.resize(len, {});
	sprites[0] = e.send ? tickSprite : Sprite();
}

LocalisedString LocUploadStringsGrid::getCellToolTip(int row, int col, const String& columnName) const
{
	if (columnName == "Group") {
		const auto& c = getChunk(row);
		return LocalisedString::fromUserString(c.chunkId);
	} else {
		const auto& e = getEntry(row);
		if (columnName == "Key") {
			return LocalisedString::fromUserString((e.oldKey ? *e.oldKey + "\n->\n" : "") + e.key);
		} else if (columnName == "Previous Value") {
			return LocalisedString::fromUserString(e.remoteValue.value_or(""));
		} else if (columnName == "New Value") {
			return LocalisedString::fromUserString(e.value);
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

LocUploadStringsWindow::LocUploadStringsWindow(UIFactory& factory, LocalisationClient& client, LocStringUploadData uploadData)
	: UIWidget("upload_strings", {}, UISizer())
	, factory(factory)
	, client(client)
	, uploadData(std::move(uploadData))
{
	setAnchor(UIAnchor());
	factory.loadUI(*this, "halley/localisation/localisation_upload_strings");
}

void LocUploadStringsWindow::onMakeUI()
{
	setStatus("Idle", Status::Idle);

	grid = std::make_shared<LocUploadStringsGrid>(factory, uploadData);
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

	client.putOriginalStrings(uploadData).then(aliveFlag, Executors::getMainUpdateThread(), [this] (bool result)
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
	bool enabled = curStatus != Status::Uploading;
	getWidget("upload")->setEnabled(enabled && sendCount > 0);
	getWidget("cancel")->setEnabled(enabled);
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

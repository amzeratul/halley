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

	Vector<float> sizes = { 20.0f, 120.0f, 40.0f, remainingWidth * 0.3f, remainingWidth * 0.7f };
	Vector<String> names { "Send", "Group", "Status", "Key", "Value" };
	return { sizes, names };
}

void LocUploadStringsGrid::getLineDrawData(int idx, Vector<String>& strs, Vector<Colour4f>& colours, Vector<Sprite>& sprites) const
{
	const auto& e = getEntry(idx);

	int len = 5;

	strs.resize(len);
	strs[1] = Path(getChunk(idx).chunkId).getFilenameStr();
	strs[2] = getTypeDesc(e.type);
	strs[3] = e.key;
	strs[4] = e.value;

	colours.resize(len, textCol);

	sprites.resize(len, {});
	sprites[0] = tickSprite;
}

LocalisedString LocUploadStringsGrid::getCellToolTip(int row, int col, const String& columnName) const
{
	if (columnName == "Group") {
		const auto& c = getChunk(row);
		return LocalisedString::fromUserString(c.chunkId);
	} else {
		const auto& e = getEntry(row);
		if (columnName == "Key") {
			return LocalisedString::fromUserString(e.key);
		} else if (columnName == "Value") {
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
	Logger::logInfo("Got " + toString(mapping.size()) + " lines in mapping");
}

const LocStringUploadChunkData& LocUploadStringsGrid::getChunk(int idx) const
{
	const auto i = mapping[idx];
	return uploadData.getChunks()[i.first];	
}

const LocStringUploadChunkData::Entry& LocUploadStringsGrid::getEntry(int idx) const
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
		return Colour4f(0.2f, 1.0f, 0.2f, 0.2f);
	case LocStringUploadEntryType::Renamed:
		return Colour4f(0.2f, 0.2f, 1.0f, 0.2f);
	case LocStringUploadEntryType::Removed:
		return Colour4f(1.0f, 0.2f, 0.2f, 0.2f);
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
	getWidget("gridContainer")->add(grid, 1);

	setHandle(UIEventType::ButtonClicked, "upload", [this] (const UIEvent& event) {
		upload();
	});
	
	setHandle(UIEventType::ButtonClicked, "cancel", [this] (const UIEvent& event) {
		destroy();
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
	bool enabled = status != Status::Uploading;
	getWidget("upload")->setEnabled(enabled);
	getWidget("cancel")->setEnabled(enabled);
	
	auto statusLabel = getWidgetAs<UILabel>("status");
	statusLabel->setText(LocalisedString::fromUserString("Status: " + message));
	statusLabel->setColour(status == Status::Error ? Colour4f::fromHexString("#ff7e7e") : Colour4f::fromHexString("#9d9d9d"));
}

void LocUploadStringsWindow::updateSummary()
{
	auto label = getWidgetAs<UILabel>("summary");

	HashMap<LocStringUploadEntryType, int> counts;
	for (const auto& c: uploadData.getChunks()) {
		for (const auto& e: c.entries) {
			++counts[e.type];
		}
	}

	using enum LocStringUploadEntryType;
	label->setText(LocalisedString::fromUserString(String("Summary: ") 
		+ counts[Added] + " added, "
		+ counts[Modified] + " modified, "
		+ counts[Removed] + " removed, "
		+ counts[Renamed] + " renamed"
	));
}

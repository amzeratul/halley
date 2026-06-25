#include "localisation_export_window.h"

using namespace Halley;

LocalisationExportWindow::LocalisationExportWindow(UIFactory& factory, Vector<String> chunks, Callback callback)
	: UIWidget("localisation_export_window", {}, UISizer())
	, factory(factory)
	, callback(std::move(callback))
	, chunks(std::move(chunks))
	, filterController(*this, filters)
{
	chunksToExport.insert(this->chunks.begin(), this->chunks.end());

	filters.initialise(true);

	factory.loadUI(*this, "halley/localisation/localisation_export");
	UIWidget::setAnchor(UIAnchor());
}

void LocalisationExportWindow::onMakeUI()
{
	setHandle(UIEventType::ButtonClicked, "export", [=] (const UIEvent& event) {
		onExport();
	});

	setHandle(UIEventType::ButtonClicked, "cancel", [=] (const UIEvent& event) {
		callback(false, {});
		destroy();
	});

	setHandle(UIEventType::ButtonClicked, "groupSelectAll", [=] (const UIEvent& event) {
		setAllChunks(true);
	});

	setHandle(UIEventType::ButtonClicked, "groupSelectNone", [=] (const UIEvent& event) {
		setAllChunks(false);
	});

	setHandle(UIEventType::CheckboxUpdated, [=] (const UIEvent& event) {
		const auto& id = event.getSourceId();
		if (id.startsWith("checkbox_")) {
			setChunkEnabled(id.mid(9), event.getBoolData());
		}
	});

	bindData("includeBOM", includeBOM, [this] (bool value) {
		includeBOM = value;
	});

	filterController.setup();

	populateChunkList();
}

void LocalisationExportWindow::update(Time t, bool moved)
{
	getWidget("export")->setEnabled(!chunksToExport.empty());
}

void LocalisationExportWindow::populateChunkList()
{
	const auto container = getWidget("stringGroups");
	const auto style = factory.getStyle("checkbox");
	for (const auto& chunk: chunks) {
		auto checkbox = std::make_shared<UICheckbox>("checkbox_" + chunk, style, chunksToExport.contains(chunk), LocalisedString::fromUserString(chunk));
		container->add(checkbox);
	}
}

void LocalisationExportWindow::setAllChunks(bool enabled)
{
	acceptChunkUpdates = false;

	if (enabled) {
		chunksToExport.clear();
		chunksToExport.insert(this->chunks.begin(), this->chunks.end());
	} else {
		chunksToExport = {};
	}

	for (const auto& chunk: chunks) {
		getWidgetAs<UICheckbox>("checkbox_" + chunk)->setChecked(enabled);
	}

	acceptChunkUpdates = true;
}

void LocalisationExportWindow::setChunkEnabled(const String& chunkId, bool enabled)
{
	if (acceptChunkUpdates) {
		if (enabled) {
			chunksToExport.insert(chunkId);
		} else {
			chunksToExport.erase(chunkId);
		}
	}
}

void LocalisationExportWindow::onExport()
{
	LocalisationExportOptions options;
	options.chunksToInclude = std::move(chunksToExport);
	options.allChunks = false;
	options.filters = filters;
	options.includeBOM = includeBOM;
	callback(true, options);

	destroy();
}
